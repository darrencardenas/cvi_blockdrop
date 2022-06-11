//==============================================================================
//
// Title:       cvi_blockdrop.h
// Purpose:     A block drop puzzle game written for LabWindows/CVI.
//
// Created on:  4/17/2022 by Darren Cardenas
//
//==============================================================================

#ifndef __cvi_blockdrop_H__
#define __cvi_blockdrop_H__

#ifdef __cplusplus
    extern "C" {
char *void;
#endif

//==============================================================================
// Include files

#include "windows.h"
#include <mmsystem.h>
#include "cvidef.h"
#include <cvirte.h>
#include <userint.h>
#include <ansi_c.h>
#include <utility.h>
#include "toolbox.h"
#include "cvi_blockdrop_UIR.h"

//==============================================================================
// Constants
        
#define DROP_DELAY              0.4  // seconds

// Sound effects
#define FOLDER_SFX              "sfx\\"
#define SFX_CLEAR_LINE          FOLDER_SFX "clear_line.wav"
#define SFX_CLEAR_TETRIS        FOLDER_SFX "clear_tetris.wav"
#define SFX_ROTATE              FOLDER_SFX "rotate.wav"
#define SFX_GAME_OVER           FOLDER_SFX "game_over.wav"

// Playing area
#define GRID_NUM_ROWS           23  // Only lower 20 rows are visible
#define GRID_NUM_COLS           10
            
// Blocks 
#define BLOCK_RANDOM            'R'
#define BLOCK_I                 'I'
#define BLOCK_J                 'J'
#define BLOCK_L                 'L'
#define BLOCK_O                 'O'
#define BLOCK_S                 'S'
#define BLOCK_T                 'T'
#define BLOCK_Z                 'Z'  
#define NUM_BLOCKS_TYPES        7  // Exclude random block    
#define NUM_SQUARES_PER_BLOCK   4
#define START_BLOCK             BLOCK_RANDOM
#define FIRST_BLOCK_NO          0
#define FIRST_BLOCK_YES         1
        
// Orientation
#define ORIENTATION_1           1  // Spawn orientation
#define ORIENTATION_2           2  // After first clockwise (CW) rotation
#define ORIENTATION_3           3  // After second clockwise rotation
#define ORIENTATION_4           4  // After third clockwise rotation

// Game conditions
#define GAME_RUN                0
#define GAME_END                1

//==============================================================================
// Types

typedef struct
{
    char type;
    int color;
    int orientation;
    Point position[NUM_SQUARES_PER_BLOCK];
    int num_rows;
    int rows[NUM_SQUARES_PER_BLOCK];
    int num_low_points;
    Point low_points[NUM_SQUARES_PER_BLOCK];
    int num_left_points;
    Point left_points[NUM_SQUARES_PER_BLOCK];
    int num_right_points;
    Point right_points[NUM_SQUARES_PER_BLOCK];
        
} blockData;
        
//==============================================================================
// External variables

//==============================================================================
// Global functions

int CheckForLineClears (void);
int ClearGrid (void);
int SpawnBlock (int first_block);
int AdvanceBlock (void);

#ifdef __cplusplus
    }
#endif

#endif  /* ndef __cvi_blockdrop_H__ */
