#pragma once


namespace Auric::Constants {

// number of floats per vertex
inline constexpr int VERTEX_FLOATS = 15;
inline constexpr int UI_FLOATS = 4;

inline constexpr double PI = 3.141592653589793;

// maximum bones because I store bone indices as a single
// unsigned int so each bone has to be less than 8 bits.
const int MAX_BONES = 128;

inline constexpr unsigned int MESHID = 1;
inline constexpr unsigned int CAMERAID = 2;
inline constexpr unsigned int RIGID = 3;
inline constexpr unsigned int LIGHTMAPID = 4;
inline constexpr unsigned int COLLIDERID = 5;
inline constexpr unsigned int CAPSULECOLLIDERID = 6;
inline constexpr unsigned int AUDIOSOURCEID = 7;
inline constexpr unsigned int SCRIPTID = 8;

}
