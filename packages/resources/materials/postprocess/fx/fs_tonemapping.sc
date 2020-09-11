$input v_texcoord0

#include <bgfx_shader.sh>
#include "common/postprocess.sh"

uniform vec4 u_exposure;

void main()
{
    vec4 c              = texture2D(s_postprocess_input, v_texcoord0);
    gl_FragColor.rgb = vec3_splat(1.0) - exp(-c.rgb * u_exposure.x);
    gl_FragColor.a   = saturate(c.a);
}