#pragma once  

#include <includes.h>

class vec3_t;

class v_matrix {
public:
    auto operator[](int i) const { return m[i]; }

    float m[4][4];
};


class matrix2x4_t {  
public:  
   vec3_t get_origin( int index );
   inline void set_origin( int index, vec3_t vec );

   union {  
       struct {  
           float _11, _12, _13, _14;  
           float _21, _22, _23, _24;  
       };  
   };  
};  

using matrix3x3_t = float[ 3 ][ 3 ];  

class matrix3x4_t {  
public:  
   matrix3x4_t( ) = default;  

   float arr_data[ 3 ][ 4 ] = {};  

   constexpr matrix3x4_t(  
       float m00, float m01, float m02, float m03,  
       float m10, float m11, float m12, float m13,  
       float m20, float m21, float m22, float m23 ) {  
       arr_data[ 0 ][ 0 ] = m00;  
       arr_data[ 0 ][ 1 ] = m01;  
       arr_data[ 0 ][ 2 ] = m02;  
       arr_data[ 0 ][ 3 ] = m03;  
       arr_data[ 1 ][ 0 ] = m10;  
       arr_data[ 1 ][ 1 ] = m11;  
       arr_data[ 1 ][ 2 ] = m12;  
       arr_data[ 1 ][ 3 ] = m13;  
       arr_data[ 2 ][ 0 ] = m20;  
       arr_data[ 2 ][ 1 ] = m21;  
       arr_data[ 2 ][ 2 ] = m22;  
       arr_data[ 2 ][ 3 ] = m23;  
   }  

   matrix3x4_t( const vec3_t& vec_forward, const vec3_t& vec_left, const vec3_t& vec_up, const vec3_t& vec_origin ) {  
       set_forward( vec_forward );  
       set_left( vec_left );  
       set_up( vec_up );  
       set_origin( vec_origin );  
   }  

   float* operator[]( int index ) {  
       return arr_data[ index ];  
   }  

   const float* operator[]( int index ) const {  
       return arr_data[ index ];  
   }  

   void set_forward( const vec3_t& vec_forward );
   void set_left( const vec3_t& vec_left );
   void set_up( const vec3_t& vec_up );
   void set_origin( const vec3_t& vec_origin );

   vec3_t forward( ) const;
   vec3_t left( ) const;
   vec3_t up( ) const;
   vec3_t origin( ) const;

   void invalidate( ) {  
       for ( auto& sub_data : arr_data ) {  
           for ( auto& data : sub_data )  
               data = -FLT_MAX;  
       }  
   }  

   matrix3x4_t concat_transforms( const matrix3x4_t& other ) const {  
       return {  
           arr_data[ 0 ][ 0 ] * other.arr_data[ 0 ][ 0 ] + arr_data[ 0 ][ 1 ] * other.arr_data[ 1 ][ 0 ] + arr_data[ 0 ][ 2 ] * other.arr_data[ 2 ][ 0 ],  
           arr_data[ 0 ][ 0 ] * other.arr_data[ 0 ][ 1 ] + arr_data[ 0 ][ 1 ] * other.arr_data[ 1 ][ 1 ] + arr_data[ 0 ][ 2 ] * other.arr_data[ 2 ][ 1 ],  
           arr_data[ 0 ][ 0 ] * other.arr_data[ 0 ][ 2 ] + arr_data[ 0 ][ 1 ] * other.arr_data[ 1 ][ 2 ] + arr_data[ 0 ][ 2 ] * other.arr_data[ 2 ][ 2 ],  
           arr_data[ 0 ][ 0 ] * other.arr_data[ 0 ][ 3 ] + arr_data[ 0 ][ 1 ] * other.arr_data[ 1 ][ 3 ] + arr_data[ 0 ][ 2 ] * other.arr_data[ 2 ][ 3 ] + arr_data[ 0 ][ 3 ],  

           arr_data[ 1 ][ 0 ] * other.arr_data[ 0 ][ 0 ] + arr_data[ 1 ][ 1 ] * other.arr_data[ 1 ][ 0 ] + arr_data[ 1 ][ 2 ] * other.arr_data[ 2 ][ 0 ],  
           arr_data[ 1 ][ 0 ] * other.arr_data[ 0 ][ 1 ] + arr_data[ 1 ][ 1 ] * other.arr_data[ 1 ][ 1 ] + arr_data[ 1 ][ 2 ] * other.arr_data[ 2 ][ 1 ],  
           arr_data[ 1 ][ 0 ] * other.arr_data[ 0 ][ 2 ] + arr_data[ 1 ][ 1 ] * other.arr_data[ 1 ][ 2 ] + arr_data[ 1 ][ 2 ] * other.arr_data[ 2 ][ 2 ],  
           arr_data[ 1 ][ 0 ] * other.arr_data[ 0 ][ 3 ] + arr_data[ 1 ][ 1 ] * other.arr_data[ 1 ][ 3 ] + arr_data[ 1 ][ 2 ] * other.arr_data[ 2 ][ 3 ] + arr_data[ 1 ][ 3 ],  

           arr_data[ 2 ][ 0 ] * other.arr_data[ 0 ][ 0 ] + arr_data[ 2 ][ 1 ] * other.arr_data[ 1 ][ 0 ] + arr_data[ 2 ][ 2 ] * other.arr_data[ 2 ][ 0 ],  
           arr_data[ 2 ][ 0 ] * other.arr_data[ 0 ][ 1 ] + arr_data[ 2 ][ 1 ] * other.arr_data[ 1 ][ 1 ] + arr_data[ 2 ][ 2 ] * other.arr_data[ 2 ][ 1 ],  
           arr_data[ 2 ][ 0 ] * other.arr_data[ 0 ][ 2 ] + arr_data[ 2 ][ 1 ] * other.arr_data[ 1 ][ 2 ] + arr_data[ 2 ][ 2 ] * other.arr_data[ 2 ][ 2 ],  
           arr_data[ 2 ][ 0 ] * other.arr_data[ 0 ][ 3 ] + arr_data[ 2 ][ 1 ] * other.arr_data[ 1 ][ 3 ] + arr_data[ 2 ][ 2 ] * other.arr_data[ 2 ][ 3 ] + arr_data[ 2 ][ 3 ]  
       };  
   }  
};