#define LUA_LIB 1
#include <lua.hpp>

#define USE_BGFX 1
#ifdef USE_BGFX
#include "../bgfx/bgfx_interface.h"
#endif //USE_BGFX

#define LIGHTMAPPER_IMPLEMENTATION
#include "lightmapper.h"

#include "lua2struct.h"
#include "luabgfx.h"

struct context{
    lm_context *lm_ctx;
    int size;
    float z_near, z_far;
    float rgb[3];
    int interp_pass_count;
    float interp_threshold;
    float cam2surf_dis_modifier;
};

LUA2STRUCT(context, size, z_near, z_far, rgb, interp_pass_count, interp_threshold, cam2surf_dis_modifier);

struct lightmap{
    float* data;
    int width;
    int height;
    int channels;
    int sizebytes() const { return width * height * channels * sizeof(float);}
};

LUA2STRUCT(lightmap, width, height, channels);

struct geometry{
    const float* worldmat;
    int num;
    struct attrib{
        union {
            const struct memory* m;
            const void *nd;
        };
        
        const void* data() const {
            return (const uint8_t*)(native ? nd : m->data) + offset;
        };
        int offset;
        int stride;
        lm_type type;
        bool native;
    };
    attrib pos;
    attrib normal;
    attrib uv;

    attrib index;
};

namespace lua_struct{
    template <>
    inline void unpack<geometry::attrib>(lua_State* L, int idx, geometry::attrib& v, void*) {
        luaL_checktype(L, idx, LUA_TTABLE);
        lua_getfield(L, idx, "native");
        v.native = lua_toboolean(L, -1);
        lua_pop(L, 1);
        const auto datatype = lua_getfield(L, idx, "memory");
        if (v.native){
            v.nd = lua_touserdata(L, -1);
        } else {
            v.m = (const struct memory*)luaL_testudata(L, -1, "BGFX_MEMORY");
        }
        
        lua_pop(L, 1);

        unpack_field(L, idx, "offset", v.offset);
        unpack_field(L, idx, "stride", v.stride);

        lua_getfield(L, idx, "type");
        auto s = lua_tostring(L, -1);
        switch (s[0]){
            case 'B': v.type = LM_UNSIGNED_BYTE; break;
            case 'H': v.type = LM_UNSIGNED_SHORT; break;
            case 'I': v.type = LM_UNSIGNED_INT; break;
            case 'f': v.type = LM_FLOAT; break;
            case '\0':v.type = LM_NONE; break;
            default: luaL_error(L, "invalid data type:%s", s);
        }
        lua_pop(L, 1);
    }

    template<>
    inline void unpack<geometry>(lua_State* L, int idx, geometry& g, void*){
        unpack_field(L, idx, "num", g.num);
        unpack_field(L, idx, "worldmat", g.worldmat);
        unpack_field(L, idx, "pos", g.pos);
        unpack_field(L, idx, "normal", g.normal);
        unpack_field(L, idx, "uv", g.uv);

        auto t = lua_getfield(L, idx, "index");
        lua_pop(L, 1);

        if (t == LUA_TTABLE){
            unpack_field(L, idx, "index", g.index);
        }else{
            g.index.type = LM_NONE;
            g.index.m = nullptr;
            g.index.stride = 0;
            g.index.offset = 0;
        }
    }
}

//TODO: we should remove all render relate code from lightmapper.h
struct shadinginfo{
    struct viewid_range{
        bgfx_view_id_t base;
        uint16_t count;
    } viewids;
    
    uint16_t storage_viewid;
    struct downsampleT{
        uint16_t prog;
        uint16_t hemispheres;
    };

    struct weight_downsampleT : public downsampleT{
        uint16_t weights;
    };

    weight_downsampleT weight_downsample;
    downsampleT downsample;
};

LUA2STRUCT(shadinginfo::weight_downsampleT, prog, hemispheres, weights);
LUA2STRUCT(shadinginfo::downsampleT, prog, hemispheres);
LUA2STRUCT(shadinginfo::viewid_range, base, count);
LUA2STRUCT(shadinginfo, viewids, storage_viewid, weight_downsample, downsample);

static inline context*
tocontext(lua_State *L, int index=1){
    return (context*)luaL_checkudata(L, index, "LM_CONTEXT_MT");
}

static int
lcontext_destroy(lua_State *L){
    auto ctx = tocontext(L);
    lmDestroy(ctx->lm_ctx);
    return 0;
}

static int lcontext_set_target_lightmap(lua_State *L);

static int
lcontext_set_geometry(lua_State *L){
    auto ctx = tocontext(L, 1);
    geometry g;
    lua_struct::unpack(L, 2, g);

    lmSetGeometry(ctx->lm_ctx, g.worldmat, 
        g.pos.type, g.pos.data(), g.pos.stride,
        g.normal.type, g.normal.data(),
        g.normal.stride, g.uv.type, g.uv.data(), g.uv.stride,
        g.num, g.index.type, g.index.data());
    return 0;
}

static int
lcontext_set_shadering_info(lua_State *L){
    auto ctx = tocontext(L, 1);
    shadinginfo si;
    lua_struct::unpack(L, 2, si);

    if (si.viewids.count == 0)
        return luaL_error(L, "invalid viewid count: 0");

    if ((si.viewids.base + si.viewids.count-1) >= si.storage_viewid){
        return luaL_error(L, "storage_viewid:%d should larger than viewid:%d, blit operation should after draw", 
            si.storage_viewid, (si.viewids.base + si.viewids.count-1));
    }

    if (std::pow(2, si.viewids.count) < ctx->lm_ctx->hemisphere.size){
        return luaL_error(L, "not enough viewid for downsample, hemisphere size:%d, viewid count: %d", ctx->lm_ctx->hemisphere.size, si.viewids.count);
    }

    lmSetDownsampleShaderingInfo(ctx->lm_ctx, si.viewids.base, si.viewids.count, si.storage_viewid,
        {si.weight_downsample.prog}, {si.weight_downsample.hemispheres}, {si.weight_downsample.weights},
        {si.downsample.prog}, {si.downsample.hemispheres});

    return 0;
}

static int
lcontext_begin_patch(lua_State *L){
    auto ctx = tocontext(L, 1);
    auto vp = (float*)lua_touserdata(L, 2);
    auto viewmat = (float*)lua_touserdata(L, 3);
    auto projmat = (float*)lua_touserdata(L, 4);
    int vp_int[4];
    lua_pushboolean(L, lmBegin(ctx->lm_ctx, vp_int, viewmat, projmat));
    for (int ii=0; ii<4; ++ii)
        vp[ii] = (float)vp_int[ii];
    return 1;
}

static int
lcontext_end_patch(lua_State *L){
    auto ctx = tocontext(L);
    lmEnd(ctx->lm_ctx);
    return 0;
}

static int
lcontext_process(lua_State *L){
    lua_pushnumber(L, lmProgress(tocontext(L, 1)->lm_ctx));
    return 1;
}

static void
register_lm_context_mt(lua_State *L){
    if (luaL_newmetatable(L, "LM_CONTEXT_MT")){
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");

        luaL_Reg l[] = {
            {"__gc", lcontext_destroy},
            {"set_target_lightmap", lcontext_set_target_lightmap},
            {"set_geometry",        lcontext_set_geometry},
            {"set_shadering_info",  lcontext_set_shadering_info},
            {"begin_patch",         lcontext_begin_patch},
            {"end_patch",           lcontext_end_patch},
            {"process",             lcontext_process},
            {nullptr,               nullptr},
        };

        luaL_setfuncs(L, l, 0);
    }
}

static int
llightmap_create_context(lua_State *L){
    auto ctx = (context*)lua_newuserdata(L, sizeof(context));
    if (LUA_TTABLE != luaL_getmetatable(L, "LM_CONTEXT_MT")){
        return luaL_error(L, "LM_CONTEXT_MT not register");
    }
    lua_setmetatable(L, -2);

    lua_struct::unpack(L, 1, *ctx);
    ctx->lm_ctx = lmCreate(ctx->size, ctx->z_near, ctx->z_far,
        ctx->rgb[0], ctx->rgb[1], ctx->rgb[2],
        ctx->interp_pass_count, ctx->interp_threshold, 
        ctx->cam2surf_dis_modifier);
    return 1;
}

static inline lightmap*
tolm(lua_State *L, int index){
    return (lightmap*)luaL_checkudata(L, index, "LIGHTMAP_MT");
}

static int
llm_destroy(lua_State *L){
    auto lm = tolm(L, 1);
    delete[] lm->data;
    return 0;
}

static int
llm_postprocess(lua_State *L){
    auto lm = tolm(L, 1);
    auto dilate_times = (int)luaL_optinteger(L, 2, 16);
    auto gamma_correct = lua_isnoneornil(L, 3) ? true : lua_toboolean(L, 3);
    // postprocess texture
	float *temp = new float[lm->sizebytes()];
    float *data = lm->data;
	for (int i = 0; i < dilate_times; i++)
	{
		lmImageDilate(data, temp, lm->width, lm->height, lm->channels);
		lmImageDilate(temp, data, lm->width, lm->height, lm->channels);
	}
	lmImageSmooth(data, temp, lm->width, lm->height, lm->channels);
	lmImageDilate(temp, data, lm->width, lm->height, lm->channels);
    if (gamma_correct){
        lmImagePower(data, lm->width, lm->height, lm->channels, 1.0f / 2.2f, 0x7); // gamma correct color channels
    }
	
	delete[] temp;
    return 0;
}

static int
llm_tostring(lua_State *L){
    auto lm = tolm(L, 1);
    const auto size = lm->sizebytes();
    lua_pushlstring(L, (const char*)lm->data, size);
    lua_pushinteger(L, size);
    return 2;
}
#ifdef _DEBUG
static int
llm_save(lua_State *L){
    auto lm = tolm(L, 1);
    auto fn = lua_tostring(L, 2);
    lmImageSaveTGAf(fn, lm->data, lm->width, lm->height, lm->channels, 1.0f);
    return 0;
}
#endif 
static int
llm_data(lua_State *L){
    auto lm = tolm(L, 1);
    lua_pushlightuserdata(L, lm->data);
    return 1;
}
static int
lcontext_set_target_lightmap(lua_State *L){
    auto ctx = tocontext(L, 1);
    lightmap *lm = (lightmap*)lua_newuserdata(L, sizeof(lightmap));
    lua_struct::unpack(L, 2, *lm);

    if (luaL_newmetatable(L, "LIGHTMAP_MT")){
        lua_pushvalue(L, -1);
        lua_setfield(L, -2, "__index");

        luaL_Reg l[] = {
            {"__gc", llm_destroy},
            {"postprocess", llm_postprocess},
            {"tostring", llm_tostring},
            {"data", llm_data},
#ifdef _DEBUG
            {"save", llm_save},
#endif
            {nullptr, nullptr},
        };

        luaL_setfuncs(L, l, 0);
    }
    lua_setmetatable(L, -2);

    lm->data = new float[lm->sizebytes()];
    memset(lm->data, 0, lm->sizebytes());
    lmSetTargetLightmap(ctx->lm_ctx, lm->data, lm->width, lm->height, lm->channels);
    return 1;
}

static int
llightmap_context(lua_State *L){
    luaL_getmetatable(L, "LM_CONTEXT_MT");
    return 1;
}

static int
lligthmap_read_obj(lua_State *L){
    //static int loadSimpleObjFile(const char *filename, vertex_t **vertices, unsigned int *vertexCount, unsigned short **indices, unsigned int *indexCount)
    const char* filename = luaL_checkstring(L, 1);
	FILE *file = fopen(filename, "rt");
	if (!file)
		return 0;
	char line[1024];

	// first pass
	unsigned int np = 0, nn = 0, nt = 0, nf = 0;
	while (!feof(file))
	{
		fgets(line, 1024, file);
		if (line[0] == '#') continue;
		if (line[0] == 'v')
		{
			if (line[1] == ' ') { np++; continue; }
			if (line[1] == 'n') { nn++; continue; }
			if (line[1] == 't') { nt++; continue; }
			assert(!"unknown vertex attribute");
		}
		if (line[0] == 'f') { nf++; continue; }
		assert(!"unknown identifier");
	}
	assert(np && np == nn && np == nt && nf); // only supports obj files without separately indexed vertex attributes

	// allocate memory
	int vertexCount = np;

    float *p = (float*)lua_newuserdata(L, np * sizeof(float) * 3);
	float *n = (float*)lua_newuserdata(L, np * sizeof(float) * 3);
    float *t = (float*)lua_newuserdata(L, np * sizeof(float) * 2);

    lua_pushinteger(L, vertexCount);
	int indexCount = nf * 3;
	uint16_t *indices = (uint16_t*)lua_newuserdata(L, indexCount * sizeof(uint16_t));
    lua_pushinteger(L, indexCount);

	// second pass
	fseek(file, 0, SEEK_SET);
	unsigned int cp = 0, cn = 0, ct = 0, cf = 0;
	while (!feof(file))
	{
		fgets(line, 1024, file);
		if (line[0] == '#') continue;
		if (line[0] == 'v')
		{
			if (line[1] == ' ') { float *pp = p+(cp++*3);char *e1, *e2; pp[0] = (float)strtod(line + 2, &e1); pp[1] = (float)strtod(e1, &e2); pp[2] = (float)strtod(e2, 0); continue; }
			if (line[1] == 'n') { /*float *n = (*vertices)[cn++].n; char *e1, *e2; n[0] = (float)strtod(line + 3, &e1); n[1] = (float)strtod(e1, &e2); n[2] = (float)strtod(e2, 0);*/ continue; } // no normals needed
			if (line[1] == 't') { float *tt = t+(ct++*2);char *e1;      tt[0] = (float)strtod(line + 3, &e1); tt[1] = (float)strtod(e1, 0);                                continue; }
			assert(!"unknown vertex attribute");
		}
		if (line[0] == 'f')
		{
			unsigned short *tri = indices + cf;
			cf += 3;
			char *e1, *e2, *e3 = line + 1;
			for (int i = 0; i < 3; i++)
			{
				unsigned long pi = strtoul(e3 + 1, &e1, 10);
				assert(e1[0] == '/');
				unsigned long ti = strtoul(e1 + 1, &e2, 10);
				assert(e2[0] == '/');
				unsigned long ni = strtoul(e2 + 1, &e3, 10);
				assert(pi == ti && pi == ni);
				tri[i] = (unsigned short)(pi - 1);
			}
			continue;
		}
		assert(!"unknown identifier");
	}

	fclose(file);
	return 6;
}

extern "C"{
LUAMOD_API int
luaopen_bake(lua_State* L) {
    init_interface(L);
    register_lm_context_mt(L);
    luaL_Reg lib[] = {
        { "create_lightmap_context", llightmap_create_context},
        { "context_metatable", llightmap_context},
        #ifdef _DEBUG
        { "read_obj", lligthmap_read_obj},
        #endif 
        { nullptr, nullptr },
    };
    luaL_newlib(L, lib);
    return 1;
}
}