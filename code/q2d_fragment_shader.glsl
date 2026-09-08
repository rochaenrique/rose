#version 330 core

// FRAGMENT SHADER RECT
in vec4 frag_color;
in vec2 frag_src_pos;
in float frag_texture_id;

in vec2 frag_dest_pos;
in vec2 frag_dest_center;
in vec2 frag_dest_half_size;

in float frag_corner_radius;
in float frag_edge_softness;
in float frag_border_thickness;

out vec4 out_color;

uniform sampler2D u_textures[16];

ivec2
get_texture_size(int texture_id)
{
  switch(texture_id)
  {
    case 0 : return textureSize(u_textures[ 0], 0);
    case 1 : return textureSize(u_textures[ 1], 0);
    case 2 : return textureSize(u_textures[ 2], 0);
    case 3 : return textureSize(u_textures[ 3], 0);
    case 4 : return textureSize(u_textures[ 4], 0);
    case 5 : return textureSize(u_textures[ 5], 0);
    case 6 : return textureSize(u_textures[ 6], 0);
    case 7 : return textureSize(u_textures[ 7], 0);
    case 8 : return textureSize(u_textures[ 8], 0);
    case 9 : return textureSize(u_textures[ 9], 0);
    case 10: return textureSize(u_textures[10], 0);
    case 11: return textureSize(u_textures[11], 0);
    case 12: return textureSize(u_textures[12], 0);
    case 13: return textureSize(u_textures[13], 0);
    case 14: return textureSize(u_textures[14], 0);
    case 15: return textureSize(u_textures[15], 0);
  }
  return(ivec2(1, 1));
}

vec4
get_texture_sample(int texture_id, vec2 uv)
{
  switch(texture_id)
  {
    case 0 : return texture(u_textures[ 0], uv);
    case 1 : return texture(u_textures[ 1], uv);
    case 2 : return texture(u_textures[ 2], uv);
    case 3 : return texture(u_textures[ 3], uv);
    case 4 : return texture(u_textures[ 4], uv);
    case 5 : return texture(u_textures[ 5], uv);
    case 6 : return texture(u_textures[ 6], uv);
    case 7 : return texture(u_textures[ 7], uv);
    case 8 : return texture(u_textures[ 8], uv);
    case 9 : return texture(u_textures[ 9], uv);
    case 10: return texture(u_textures[10], uv);
    case 11: return texture(u_textures[11], uv);
    case 12: return texture(u_textures[12], uv);
    case 13: return texture(u_textures[13], uv);
    case 14: return texture(u_textures[14], uv);
    case 15: return texture(u_textures[15], uv);
  }
  return(vec4(1.0, 1.0, 1.0, 1.0)); //debug color
}


float 
rounded_rect_sdf(vec2 sample_pos, vec2 rect_center, vec2 rect_half_size, float r)
{
  vec2 d2 = (abs(rect_center - sample_pos) - rect_half_size + vec2(r, r));
  float result = min(max(d2.x, d2.y), 0.0) + length(max(d2, 0.0)) - r;
  return result;
}

void 
main()
{
  vec2 softness_padding = vec2(max(0, frag_edge_softness*2-1),
                               max(0, frag_edge_softness*2-1));
  
  float border_factor = 1.f;
  if(frag_border_thickness != 0.0f)
  {
    vec2 interior_half_size = frag_dest_half_size - vec2(frag_border_thickness);
    
    float interior_radius_reduce_f = min(interior_half_size.x/frag_dest_half_size.x, 
                                         interior_half_size.y/frag_dest_half_size.y);
    float interior_corner_radius = (frag_corner_radius * interior_radius_reduce_f * interior_radius_reduce_f);
    
    float inside_d = rounded_rect_sdf(frag_dest_pos, 
                                      frag_dest_center,
                                      interior_half_size - softness_padding,
                                      interior_corner_radius);
    float inside_f = smoothstep(0, 2*frag_edge_softness, inside_d);
    border_factor = inside_f;
  }
  
  float dist = rounded_rect_sdf(frag_dest_pos, 
                                frag_dest_center,
                                frag_dest_half_size - softness_padding,
                                frag_corner_radius);
  
  float sdf_factor = 1.f - smoothstep(0, 2*frag_edge_softness, dist);
  
  ivec2 texture_size = get_texture_size(int(frag_texture_id));
  vec2 uv = vec2(frag_src_pos.x / texture_size.x, frag_src_pos.y / texture_size.y);
  vec4 sample = get_texture_sample(int(frag_texture_id), uv);
  
  out_color = frag_color * sample * sdf_factor * border_factor;
}
