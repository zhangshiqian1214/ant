
//#include "../common/common.sh"
  
uniform vec4 u_params1;
#define u_shadowMapOffset u_params1.y

uniform mat4 u_lightMtx;
uniform mat4 u_shadowMapMtx0;
uniform mat4 u_shadowMapMtx1;
uniform mat4 u_shadowMapMtx2;
uniform mat4 u_shadowMapMtx3;



/*
void main() 
{
	gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0) );
	vec4 normal = a_normal;  		//* 2.0 - 1.0;
	normal = normalize(normal);   	// 归一化测试
	
	v_normal = normalize(mul(u_modelView, vec4(normal.xyz, 0.0) ).xyz);
	v_view = mul(u_modelView, vec4(a_position, 1.0)).xyz;

	vec4 posOffset = vec4(a_position + normal.xyz * u_shadowMapOffset, 1.0);
	
	v_position = mul(u_modelView, posOffset);

	vec4 wpos = vec4(mul(u_model[0], posOffset).xyz, 1.0);    //worldspace 
	v_texcoord1 = mul(u_shadowMapMtx0, wpos);
	v_texcoord2 = mul(u_shadowMapMtx1, wpos);
	v_texcoord3 = mul(u_shadowMapMtx2, wpos);
	v_texcoord4 = mul(u_shadowMapMtx3, wpos);
}
*/
