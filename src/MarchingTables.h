/*
 * MarchingTables.h
 *
 * Common lookup tables for Marching Tetrahedra.
 *
 * A voxel cube is decomposed into six tetrahedra. Each tetrahedron is
 * processed independently.
 */

#ifndef SRC_MARCHINGTABLES_H_
#define SRC_MARCHINGTABLES_H_

namespace MarchingTables
{

// ---------------------------------------------------------------------------
// Cube corner numbering
//
//        7--------6
//       /|       /|
//      4--------5 |
//      | |      | |
//      | 3------|-2
//      |/       |/
//      0--------1
//
// ---------------------------------------------------------------------------

// Offsets of the eight cube corners.
static const int CubeCornerOffset[8][3] =
{
    {0,0,0},
    {1,0,0},
    {1,1,0},
    {0,1,0},
    {0,0,1},
    {1,0,1},
    {1,1,1},
    {0,1,1}
};

// ---------------------------------------------------------------------------
// Six tetrahedra forming one cube.
//
// Each row contains the four cube-corner indices belonging to one tetrahedron.
//
// ---------------------------------------------------------------------------

static const int CubeTetrahedra[6][4] =
{
    {0,5,1,6},
    {0,1,2,6},
    {0,2,3,6},
    {0,3,7,6},
    {0,7,4,6},
    {0,4,5,6}
};

// ---------------------------------------------------------------------------
// Local edge numbering inside one tetrahedron.
//
//          3
//         /|\
//        / | \
//      5/  |2 \4
//      /   |   \
//     0----|----1
//      \   |   /
//      0\  |1 /
//        \ | /
//         \|/
//          2
//
// ---------------------------------------------------------------------------

static const int TetraEdges[6][2] =
{
    {0,1},
    {1,2},
    {2,0},
    {0,3},
    {1,3},
    {2,3}
};

// ---------------------------------------------------------------------------
// Triangle cases.
//
// There are only 16 tetrahedron configurations (2^4 corner states).
//
// The implementation will compute intersections procedurally, so no
// copyrighted lookup tables are required.
//
// ---------------------------------------------------------------------------

enum TetraCase
{
    CASE_0  = 0,
    CASE_1  = 1,
    CASE_2  = 2,
    CASE_3  = 3,
    CASE_4  = 4,
    CASE_5  = 5,
    CASE_6  = 6,
    CASE_7  = 7,
    CASE_8  = 8,
    CASE_9  = 9,
    CASE_10 = 10,
    CASE_11 = 11,
    CASE_12 = 12,
    CASE_13 = 13,
    CASE_14 = 14,
    CASE_15 = 15
};

} // namespace MarchingTables

#endif /* SRC_MARCHINGTABLES_H_ */
