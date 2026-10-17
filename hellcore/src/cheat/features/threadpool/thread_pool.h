#pragma once

#include <vector>
#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <stdexcept>
#include <optional>
#include <windows.h>
#include <immintrin.h>

class c_threadpool;

namespace detail {

    class xorshift_prng {
        uint32_t state;
    public:
        xorshift_prng( ) : state( std::random_device{}() ^ 0x1337c0de ) {}

        uint32_t next( ) {
            state ^= state << 13;
            state ^= state >> 17;
            state ^= state << 5;
            return state;
        }
    };

    struct task_base {
        virtual ~task_base( ) = default;
        virtual void execute( ) = 0;
    };

    template <typename F>
    struct task_impl final : task_base {
        F func;
        explicit task_impl( F&& f ) noexcept : func( std::move( f ) ) {}
        void execute( ) override { func( ); }
    };

    struct alignas(64) task_container {

        alignas(std::max_align_t) std::byte storage[sizeof( task_impl<void(*)()> ) + 64];

        task_base* task = nullptr;
        task_container* next_free = nullptr;
    };


    template<typename T, size_t Size>
    class alignas(64) work_stealing_queue {
        static_assert((Size > 0) && ((Size& (Size - 1)) == 0), "work_stealing_queue size must be a power of two");

        std::vector<std::atomic<T*>> m_buffer;
        alignas(64) std::atomic<size_t> m_top{ 0 };
        alignas(64) std::atomic<size_t> m_bottom{ 0 };

    public:
        work_stealing_queue( ) : m_buffer( Size ) {
            for ( size_t i = 0; i < Size; ++i ) m_buffer[i].store( nullptr, std::memory_order_relaxed );
        }
        work_stealing_queue( const work_stealing_queue& ) = delete;
        work_stealing_queue& operator=( const work_stealing_queue& ) = delete;

        bool push( T* val ) {
            const size_t b = m_bottom.load( std::memory_order_relaxed );
            const size_t t = m_top.load( std::memory_order_acquire );

            if ( b - t >= Size ) {
                // Queue is full.
                return false;
            }

            m_buffer[b % Size].store( val, std::memory_order_relaxed );

            std::atomic_thread_fence( std::memory_order_release );

            m_bottom.store( b + 1, std::memory_order_relaxed );
            return true;
        }

        T* pop( ) {
            size_t b = m_bottom.load( std::memory_order_relaxed );
            if ( m_top.load( std::memory_order_relaxed ) >= b ) {
                return nullptr;
            }
            b--;
            m_bottom.store( b, std::memory_order_relaxed );

            std::atomic_thread_fence( std::memory_order_seq_cst );

            size_t t = m_top.load( std::memory_order_relaxed );

            if ( t > b ) {
                // The queue became empty after we decremented 'bottom'.
                // This means we raced with a stealer.
                m_bottom.store( b + 1, std::memory_order_relaxed ); 
                return nullptr;
            }

            T* val = m_buffer[b % Size].load( std::memory_order_relaxed );

            if ( t < b ) {
                return val;
            }

            // Only one item left, must race with stealers for it.
            // We try to increment 'top' to claim the last item.
            if ( !m_top.compare_exchange_strong( t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed ) ) {
                // A stealer won the race.
                val = nullptr;
            }

            // We successfully popped or lost the race. Either way, the queue is now empty.
            // We must reset bottom to its original state before this failed pop.
            m_bottom.store( b + 1, std::memory_order_relaxed );
            return val;
        }

        T* steal( ) {
            size_t t = m_top.load( std::memory_order_acquire );

            // This fence ensures that our read of 'top' happens before our read of 'bottom'.
            std::atomic_thread_fence( std::memory_order_seq_cst );

            size_t b = m_bottom.load( std::memory_order_acquire );

            if ( t >= b ) {
                // Queue is empty.
                return nullptr;
            }

            T* val = m_buffer[t % Size].load( std::memory_order_acquire );

            // Attempt to claim the item by incrementing 'top'.
            if ( !m_top.compare_exchange_strong( t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed ) ) {
                // Another stealer beat us to it.
                return nullptr;
            }

            return val;
        }
    };
} // namespace detail

class alignas(64) c_threadpool {
public:
    explicit c_threadpool( size_t threads = 0 ) {
        if ( threads == 0 ) {
            threads = std::thread::hardware_concurrency( );
        }
        m_thread_count = std::max<size_t>( 1, threads );

        m_iocp = CreateIoCompletionPort( INVALID_HANDLE_VALUE, nullptr, 0, 0 );
        if ( !m_iocp ) {
            throw std::runtime_error( "[threadpool] CreateIoCompletionPort failed" );
        }

        HMODULE tier0 = GetModuleHandleA( "tier0.dll" );
        auto create_simple_thread = tier0 ?
            reinterpret_cast<void* (__cdecl*)(void*, void*, DWORD*, int, int, void*, void*)>(GetProcAddress( tier0, "CreateSimpleThread" ))
            : nullptr;

        m_queues.reserve( m_thread_count );
        m_thread_data.reserve( m_thread_count );

        grow_global_pool( GLOBAL_POOL_INITIAL_SIZE );

        for ( size_t i = 0; i < m_thread_count; ++i ) {
            m_queues.emplace_back( std::make_unique<queue_t>( ) );
            m_thread_data.emplace_back( std::make_unique<thread_data>( ) );

            for ( size_t j = 0; j < LOCAL_POOL_SIZE; ++j ) {
                if ( container_t* container = m_global_free_list.pop( ) ) {
                    container->next_free = m_thread_data.back( )->local_free_list_head;
                    m_thread_data.back( )->local_free_list_head = container;
                }
            }
        }

        m_custom_threads.reserve( m_thread_count );
        m_thread_init_contexts.reserve( m_thread_count ); 

        for ( size_t i = 0; i < m_thread_count; ++i ) {
            m_thread_init_contexts.push_back( { this, i } );

            if ( create_simple_thread ) {
                DWORD tid = 0;
                void* handle = create_simple_thread( &c_threadpool::thread_trampoline, &m_thread_init_contexts.back( ), &tid, 0, 0, nullptr, nullptr );
                if ( !handle ) {
                    m_stop = true;
                    for ( size_t j = 0; j < i; ++j ) PostQueuedCompletionStatus( m_iocp, 0, 0, nullptr );
                    for ( HANDLE h : m_custom_threads ) {
                        WaitForSingleObject( h, INFINITE );
                        CloseHandle( h );
                    }
                    CloseHandle( m_iocp );
                    throw std::runtime_error( "[threadpool] CreateSimpleThread failed" );
                }
                m_custom_threads.push_back( handle );
            }
            else {
                throw std::runtime_error( "[threadpool] Could not find CreateSimpleThread in tier0.dll" );
            }
        }
    }

    ~c_threadpool( ) {
        m_stop.store( true, std::memory_order_relaxed );

        for ( size_t i = 0; i < m_thread_count; ++i ) {
            PostQueuedCompletionStatus( m_iocp, 0, 0, nullptr );
        }

        for ( HANDLE h : m_custom_threads ) {
            if ( h ) {
                WaitForSingleObject( h, INFINITE );
                CloseHandle( h );
            }
        }

        if ( m_iocp ) {
            CloseHandle( m_iocp );
        }
    }

    c_threadpool( const c_threadpool& ) = delete;
    c_threadpool& operator=( const c_threadpool& ) = delete;

    template<typename F>
    void enqueue( F&& f ) {
        using task_t = detail::task_impl<F>;
        static_assert(sizeof( task_t ) <= sizeof( detail::task_container::storage ), "Task callable is too large for container storage");

        container_t* container = alloc_container( std::nullopt );
        if ( !container ) {
            throw std::runtime_error( "[threadpool] Failed to allocate task container. Pool may be exhausted." );
        }

        new (container->storage) task_t( std::forward<F>( f ) );
        container->task = reinterpret_cast<detail::task_base*>(container->storage);

        const size_t queue_idx = m_next_queue.fetch_add( 1, std::memory_order_relaxed ) % m_thread_count;

        if ( !m_queues[queue_idx]->push( container ) ) {
            bool pushed = false;
            for ( size_t i = 1; i < m_thread_count; ++i ) {
                if ( m_queues[(queue_idx + i) % m_thread_count]->push( container ) ) {
                    pushed = true;
                    break;
                }
            }
            if ( !pushed ) {
                free_container_to_global( container );
                throw std::runtime_error( "[threadpool] All worker queues are full; cannot enqueue task." );
            }
        }

        if ( m_sleeping_threads.load( std::memory_order_relaxed ) > 0 ) {
            PostQueuedCompletionStatus( m_iocp, 0, 0, nullptr );
        }
    }

    [[nodiscard]] bool try_run_one_task( ) {
        size_t victim_start_idx = m_next_queue.fetch_add( 1, std::memory_order_relaxed ) % m_thread_count;

        for ( size_t i = 0; i < m_thread_count; ++i ) {
            size_t victim_idx = (victim_start_idx + i) % m_thread_count;
            if ( container_t* task_cont = m_queues[victim_idx]->steal( ) ) {
                task_cont->task->execute( );
                free_container_to_global( task_cont );
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] size_t get_thread_count( ) const noexcept { return m_thread_count; }

private:

    static constexpr size_t QUEUE_SIZE = 256;
    static constexpr size_t LOCAL_POOL_SIZE = 128;
    static constexpr size_t GLOBAL_POOL_INITIAL_SIZE = 1024;
    static constexpr size_t POOL_GROWTH_SIZE = 256;
    static constexpr int SPIN_ATTEMPTS = 4096;

    using container_t = detail::task_container;
    using queue_t = detail::work_stealing_queue<container_t, QUEUE_SIZE>;

    struct alignas(64) thread_data {
        container_t* local_free_list_head = nullptr;
        detail::xorshift_prng prng;
    };

    struct thread_init_ctx {
        c_threadpool* pool_ptr;
        size_t thread_idx;
    };

    class alignas(64) lockfree_stack {
        std::atomic<container_t*> m_head{ nullptr };
    public:
        void push( container_t* container ) {
            auto old_head = m_head.load( std::memory_order_relaxed );
            do {
                container->next_free = old_head;
            } while ( !m_head.compare_exchange_weak( old_head, container, std::memory_order_release, std::memory_order_relaxed ) );
        }

        container_t* pop( ) {
            auto old_head = m_head.load( std::memory_order_acquire );
            do {
                if ( !old_head ) {
                    return nullptr;
                }
            } while ( !m_head.compare_exchange_weak( old_head, old_head->next_free, std::memory_order_acquire, std::memory_order_relaxed ) );
            return old_head;
        }
    };

    size_t m_thread_count = 0;
    std::vector<std::unique_ptr<queue_t>> m_queues;
    std::vector<std::unique_ptr<thread_data>> m_thread_data;

    char _pad0[64];
    std::atomic<bool> m_stop{ false };
    char _pad1[64];
    std::atomic<size_t> m_next_queue{ 0 };
    char _pad2[64];
    std::atomic<size_t> m_sleeping_threads{ 0 };
    char _pad3[64];

    HANDLE m_iocp = nullptr;
    std::vector<HANDLE> m_custom_threads;
    std::vector<thread_init_ctx> m_thread_init_contexts;

    lockfree_stack m_global_free_list;
    std::vector<std::unique_ptr<container_t>> m_owned_pool;
    std::mutex m_pool_growth_mutex;

    void grow_global_pool( size_t count ) {
        std::lock_guard<std::mutex> lock( m_pool_growth_mutex );
        const size_t original_size = m_owned_pool.size( );
        m_owned_pool.reserve( original_size + count );
        for ( size_t i = 0; i < count; ++i ) {
            auto container = std::make_unique<container_t>( );
            m_global_free_list.push( container.get( ) );
            m_owned_pool.push_back( std::move( container ) );
        }
    }

    container_t* alloc_container( std::optional<size_t> thread_idx ) {
        if ( thread_idx.has_value( ) ) {
            auto& data = m_thread_data[*thread_idx];
            if ( data->local_free_list_head ) {
                container_t* c = data->local_free_list_head;
                data->local_free_list_head = c->next_free;
                return c;
            }
        }

        if ( container_t* c = m_global_free_list.pop( ) ) {
            return c;
        }

        grow_global_pool( POOL_GROWTH_SIZE );
        return m_global_free_list.pop( );
    }

    void free_container( container_t* container, size_t thread_idx ) {
        if ( container->task ) {
            container->task->~task_base( );
            container->task = nullptr;
        }
        auto& data = m_thread_data[thread_idx];
        container->next_free = data->local_free_list_head;
        data->local_free_list_head = container;
    }

    void free_container_to_global( container_t* container ) {
        if ( container->task ) {
            container->task->~task_base( );
            container->task = nullptr;
        }
        m_global_free_list.push( container );
    }

    static void __cdecl thread_trampoline( thread_init_ctx* ctx ) {
        ctx->pool_ptr->worker_loop( ctx->thread_idx );
    }

    void worker_loop( size_t thread_idx ) {
        SetThreadAffinityMask( GetCurrentThread( ), 1ULL << thread_idx );
        SetThreadPriority( GetCurrentThread( ), THREAD_PRIORITY_TIME_CRITICAL );

        queue_t* my_queue = m_queues[ thread_idx ].get( );
        auto& my_data = m_thread_data[ thread_idx ];

        while ( !m_stop.load( std::memory_order_relaxed ) ) {
            container_t* task_cont = nullptr;

            if ( task_cont = my_queue->pop( ); !task_cont ) {
                const size_t victim_count = m_thread_count - 1;
                if ( victim_count > 0 ) {
                    size_t random_offset = my_data->prng.next( ) % m_thread_count;
                    for ( size_t i = 0; i < m_thread_count; ++i ) {
                        size_t victim_idx = ( random_offset + i ) % m_thread_count;
                        if ( victim_idx == thread_idx ) continue;

                        if ( task_cont = m_queues[ victim_idx ]->steal( ) ) {
                            break;
                        }
                    }
                }
            }

            if ( task_cont ) {
                task_cont->task->execute( );
                free_container( task_cont, thread_idx );
                continue;
            }

            for ( int i = 0; i < SPIN_ATTEMPTS; ++i ) {
                if ( ( task_cont = my_queue->pop( ) ) || ( task_cont = m_queues[ my_data->prng.next( ) % m_thread_count ]->steal( ) ) ) {
                    break;
                }
                _mm_pause( );
            }

            if ( task_cont ) {
                task_cont->task->execute( );
                free_container( task_cont, thread_idx );
                continue;
            }

            m_sleeping_threads.fetch_add( 1, std::memory_order_relaxed );
            if ( my_queue->pop( ) || try_run_one_task( ) ) {
                m_sleeping_threads.fetch_sub( 1, std::memory_order_relaxed );
                continue;
            }

            DWORD bytes; ULONG_PTR key; LPOVERLAPPED ov;
            GetQueuedCompletionStatus( m_iocp, &bytes, &key, &ov, INFINITE );
            m_sleeping_threads.fetch_sub( 1, std::memory_order_relaxed );
        }
    }
};

inline std::unique_ptr<c_threadpool> g_threadpool;

inline void start_threadpool( size_t t = 0 ) {
    if ( !g_threadpool ) {
        SetPriorityClass( GetCurrentProcess( ), HIGH_PRIORITY_CLASS ); 
        g_threadpool = std::make_unique<c_threadpool>( t );
    }
}