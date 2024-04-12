#pragma once
#define __dojostatic extern "C" __declspec(dllexport)
#define __dojoclass class //__declspec(dllexport)
#define __dojostruct struct __declspec(dllexport)

#define __FP (float*)&
#define VZRESULT int
#define VZ_OK 0
#define VZ_FAIL 1
#define VZ_JOB_WAIT 1

#define SAFE_GET_COPY(DST_PTR, SRC_PTR, TYPE, ELEMENTS) { if(DST_PTR) memcpy(DST_PTR, SRC_PTR, sizeof(TYPE)*ELEMENTS); }
#define GET_COPY(DST_PTR, SRC_PTR, TYPE, ELEMENTS) { memcpy(DST_PTR, SRC_PTR, sizeof(TYPE)*ELEMENTS); }

// std
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <set>
#include <any>
#include <list>
#include <memory>
#include <algorithm>
#include <chrono>

using VID = uint32_t;
inline constexpr VID INVALID_VID = 0;
using TimeStamp = std::chrono::high_resolution_clock::time_point;

constexpr float VZ_PI = 3.141592654f;
constexpr float VZ_2PI = 6.283185307f;
constexpr float VZ_1DIVPI = 0.318309886f;
constexpr float VZ_1DIV2PI = 0.159154943f;
constexpr float VZ_PIDIV2 = 1.570796327f;
constexpr float VZ_PIDIV4 = 0.785398163f;

using uint = uint32_t;

// DOJO TO DO (or suggestion)
// 1. separate mesh and material (mesh to geometry... and material is an option for objectcomponent)
// 2. set up a resource pool (or a scene for this?! for the geometry anbd material, animations,....)

namespace vzm
{
	__dojostatic inline void TransformPoint(const float posSrc[3], const float mat[16], const bool rowMajor, float posDst[3]);
	__dojostatic inline void TransformVector(const float vecSrc[3], const float mat[16], const bool rowMajor, float vecDst[3]);
	__dojostatic inline void ComputeBoxTransformMatrix(const float cubeScale[3], const float posCenter[3],
		const float yAxis[3], const float zAxis[3], const bool rowMajor, float mat[16], float matInv[16]);

	template <typename ID> struct ParamMap {
	private:
		std::string __PM_VERSION = "LIBI_1.4";
		std::unordered_map<ID, std::any> __params;
	public:
		bool FindParam(const ID& param_name) {
			auto it = __params.find(param_name);
			return !(it == __params.end());
		}
		template <typename SRCV> bool GetParamCheck(const ID& key, SRCV& param) {
			auto it = __params.find(key);
			if (it == __params.end()) return false;
			param = std::any_cast<SRCV&>(it->second);
			return true;
		}
		template <typename SRCV> SRCV GetParam(const ID& key, const SRCV& init_v) const {
			auto it = __params.find(key);
			if (it == __params.end()) return init_v;
			return std::any_cast<const SRCV&>(it->second);
		}
		template <typename SRCV> SRCV* GetParamPtr(const ID& key) {
			auto it = __params.find(key);
			if (it == __params.end()) return NULL;
			return (SRCV*)&std::any_cast<SRCV&>(it->second);
		}
		template <typename SRCV, typename DSTV> bool GetParamCastingCheck(const ID& key, DSTV& param) {
			auto it = __params.find(key);
			if (it == __params.end()) return false;
			param = (DSTV)std::any_cast<SRCV&>(it->second);
			return true;
		}
		template <typename SRCV, typename DSTV> DSTV GetParamCasting(const ID& key, const DSTV& init_v) {
			auto it = __params.find(key);
			if (it == __params.end()) return init_v;
			return (DSTV)std::any_cast<SRCV&>(it->second);
		}
		void SetParam(const ID& key, const std::any& param) {
			__params[key] = param;
		}
		void RemoveParam(const ID& key) {
			auto it = __params.find(key);
			if (it != __params.end()) {
				__params.erase(it);
			}
		}
		void RemoveAll() {
			__params.clear();
		}
		size_t Size() {
			return __params.size();
		}
		std::string GetPMapVersion() {
			return __PM_VERSION;
		}

		typedef std::unordered_map<ID, std::any> MapType;
		typename typedef MapType::iterator iterator;
		typename typedef MapType::const_iterator const_iterator;
		typename typedef MapType::reference reference;
		iterator begin() { return __params.begin(); }
		const_iterator begin() const { return __params.begin(); }
		iterator end() { return __params.end(); }
		const_iterator end() const { return __params.end(); }
	};

	enum class COMPONENT_TYPE
	{
		UNDEFINED = 0,
		CAMERA,
		ACTOR,
		GEOMETRY,
		MATERIAL,
		LIGHT,
		EMITTER,
		ANIMATION,
		WEATHER,
	};

	struct VmBaseComponent
	{
		VID componentVID = INVALID_VID;
		COMPONENT_TYPE compType = COMPONENT_TYPE::UNDEFINED;
		TimeStamp timeStamp = {}; // will be automatically set 
		ParamMap<std::string> attributes;
	};
	struct VmRenderer;
	struct VmCamera : VmBaseComponent
	{
		VmRenderer* renderer = nullptr;
		void SetPose(const float pos[3], const float view[3], const float up[3]);
		void SetPerspectiveProjection(const float zNearP, const float zFarP, const float fovY, const float aspectRatio);
		void SetCanvasSize(const float w, const float h, const float dpi);
		void GetPose(float pos[3], float view[3], float up[3]);
		void GetPerspectiveProjection(float* zNearP, float* zFarP, float* fovY, float* aspectRatio);
		void GetCanvasSize(float* w, float* h, float* dpi);
	};
	struct VmActor : VmBaseComponent
	{
		// mesh 에 rendering 요소가 저장된 구조...
	};
	struct VmMesh : VmBaseComponent
	{
		// mesh 가 material entity 를 갖는 구조...
	};
	struct VmMaterial : VmBaseComponent
	{
	};
	struct VmEmitter : VmBaseComponent
	{
		/*
		struct ParticleCounters
		{
			uint aliveCount;
			uint deadCount;
			uint realEmitCount;
			uint aliveCount_afterSimulation;
			uint culledCount;
			uint cellAllocator;
		};

		enum PARTICLESHADERTYPE
		{
			SOFT,
			SOFT_DISTORTION,
			SIMPLE,
			SOFT_LIGHTING,
			PARTICLESHADERTYPE_COUNT,
			ENUM_FORCE_UINT32 = 0xFFFFFFFF,
		};

		ParticleCounters statistics = {};	// just for information

		enum FLAGS
		{
			FLAG_EMPTY = 0,
			FLAG_DEBUG = 1 << 0,
			FLAG_PAUSED = 1 << 1,
			FLAG_SORTING = 1 << 2,
			FLAG_DEPTHCOLLISION = 1 << 3,
			FLAG_SPH_FLUIDSIMULATION = 1 << 4,
			FLAG_HAS_VOLUME = 1 << 5,
			FLAG_FRAME_BLENDING = 1 << 6,
			FLAG_COLLIDERS_DISABLED = 1 << 7,
			FLAG_USE_RAIN_BLOCKER = 1 << 8,
			FLAG_TAKE_COLOR_FROM_MESH = 1 << 9,
		};
		uint32_t _flags = FLAG_EMPTY;

		PARTICLESHADERTYPE shaderType = SOFT;

		VID meshID = INVALID_VID;

		float FIXED_TIMESTEP = -1.0f; // -1 : variable timestep; >=0 : fixed timestep

		float size = 1.0f;
		float random_factor = 1.0f;
		float normal_factor = 1.0f;
		float count = 0.0f;
		float life = 1.0f;
		float random_life = 1.0f;
		float scaleX = 1.0f;
		float scaleY = 1.0f;
		float rotation = 0.0f;
		float motionBlurAmount = 0.0f;
		float mass = 1.0f;
		float random_color = 0;

		XMFLOAT3 velocity = {}; // starting velocity of all new particles
		XMFLOAT3 gravity = {}; // constant gravity force
		float drag = 1.0f; // constant drag (per frame velocity multiplier, reducing it will make particles slow down over time)
		float restitution = 0.98f; // if the particles have collision enabled, then after collision this is a multiplier for their bouncing velocities

		float SPH_h = 1.0f;		// smoothing radius
		float SPH_K = 250.0f;	// pressure constant
		float SPH_p0 = 1.0f;	// reference density
		float SPH_e = 0.018f;	// viscosity constant

		// Sprite sheet properties:
		uint32_t framesX = 1;
		uint32_t framesY = 1;
		uint32_t frameCount = 1;
		uint32_t frameStart = 0;
		float frameRate = 0; // frames per second

		void SetMaxParticleCount(uint32_t value);
		uint32_t GetMaxParticleCount() const { return MAX_PARTICLES; }
		uint64_t GetMemorySizeInBytes() const;

		// Non-serialized attributes:
		XMFLOAT3 center;
		uint32_t layerMask = ~0u;
		XMFLOAT4X4 worldMatrix = wi::math::IDENTITY_MATRIX;

		bool IsDebug() const { return _flags & FLAG_DEBUG; }
		bool IsPaused() const { return _flags & FLAG_PAUSED; }
		bool IsSorted() const { return _flags & FLAG_SORTING; }
		bool IsDepthCollisionEnabled() const { return _flags & FLAG_DEPTHCOLLISION; }
		bool IsSPHEnabled() const { return _flags & FLAG_SPH_FLUIDSIMULATION; }
		bool IsVolumeEnabled() const { return _flags & FLAG_HAS_VOLUME; }
		bool IsFrameBlendingEnabled() const { return _flags & FLAG_FRAME_BLENDING; }
		bool IsCollidersDisabled() const { return _flags & FLAG_COLLIDERS_DISABLED; }
		bool IsTakeColorFromMesh() const { return _flags & FLAG_TAKE_COLOR_FROM_MESH; }

		void SetDebug(bool value) { if (value) { _flags |= FLAG_DEBUG; } else { _flags &= ~FLAG_DEBUG; } }
		void SetPaused(bool value) { if (value) { _flags |= FLAG_PAUSED; } else { _flags &= ~FLAG_PAUSED; } }
		void SetSorted(bool value) { if (value) { _flags |= FLAG_SORTING; } else { _flags &= ~FLAG_SORTING; } }
		void SetDepthCollisionEnabled(bool value) { if (value) { _flags |= FLAG_DEPTHCOLLISION; } else { _flags &= ~FLAG_DEPTHCOLLISION; } }
		void SetSPHEnabled(bool value) { if (value) { _flags |= FLAG_SPH_FLUIDSIMULATION; } else { _flags &= ~FLAG_SPH_FLUIDSIMULATION; } }
		void SetVolumeEnabled(bool value) { if (value) { _flags |= FLAG_HAS_VOLUME; } else { _flags &= ~FLAG_HAS_VOLUME; } }
		void SetFrameBlendingEnabled(bool value) { if (value) { _flags |= FLAG_FRAME_BLENDING; } else { _flags &= ~FLAG_FRAME_BLENDING; } }
		void SetCollidersDisabled(bool value) { if (value) { _flags |= FLAG_COLLIDERS_DISABLED; } else { _flags &= ~FLAG_COLLIDERS_DISABLED; } }
		void SetTakeColorFromMesh(bool value) { if (value) { _flags |= FLAG_TAKE_COLOR_FROM_MESH; } else { _flags &= ~FLAG_TAKE_COLOR_FROM_MESH; } }
		/**/
	};
	struct VmLight : VmBaseComponent
	{
	};
	struct VmWeather : VmBaseComponent
	{
		void SetWeatherPreset(const uint32_t index);
	};
	struct VmAnimation : VmBaseComponent
	{
		void Play();
		void Pause();
		void Stop();
		void SetLooped(const bool value);
	};
}
