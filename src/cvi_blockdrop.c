//==============================================================================
//
// Title:       cvi_blockdrop.c
// Purpose:     A block drop puzzle game written for LabWindows/CVI.
//
// Created on:  4/17/2022 by Darren Cardenas
//
//==============================================================================

//==============================================================================
// Include files

#include "cvi_blockdrop.h"

//==============================================================================
// Constants

//==============================================================================
// Types

//==============================================================================
// Static global variables
static int main_ph = 0;  // Main panel handle

static blockData block = {0};

static CmtThreadLockHandle threadLock = 0;

static int g_keydown = 0;

static double g_speed = NORMAL_SPEED;

static int g_startBlock = BLOCK_RANDOM;  // Normally BLOCK_RANDOM

//==============================================================================
// Static functions

//==============================================================================
// Global variables

//==============================================================================
// Global functions

int main (int argc, char *argv[])
{    
    int status = 0;
    intptr_t postinghandle = 0;
    
    if (InitCVIRTE (0, argv, 0) == 0)
    {
        return -1;    /* out of memory */
    }
        
    status = CmtNewLock (NULL, 0, &threadLock);
    if (status != 0)
    {
        MessagePopup ("Error", "Unable to create a new thread lock.");
        return -1;
    }
    
    main_ph = LoadPanelEx (0, "cvi_blockdrop_UIR.uir", PNLMAIN, __CVIUserHInst);    
    if (main_ph <= 0)
    {
        MessagePopup ("Error", "Unable to load main panel");
        return -1;
    }  
    
    // Monitor keyboard presses
    InstallWinMsgCallback (main_ph, WM_KEYDOWN, CB_KeyDown, VAL_MODE_IN_QUEUE, NULL, &postinghandle);    
    InstallWinMsgCallback (main_ph, WM_KEYUP, CB_KeyUp, VAL_MODE_IN_QUEUE, NULL, &postinghandle);    
    
    status = DisplayPanel (main_ph);
    if (status < 0)
    {
        MessagePopup ("Error", "Unable to display main panel");
        return -1;   
    }
    
    RunUserInterface ();
        
    if (main_ph > 0)
    {
        DiscardPanel (main_ph);
        main_ph = 0;
    }
    
    return 0;
    
}  // End of main()


int AdvanceBlock (void)
{
    int block_stop = 0;  // Flag to stop the active block
    int color = VAL_WHITE;
    int game_status = GAME_RUN;
    int gotLock = 0;
    int ii = 0;  // Loop iterator
    int status = 0;
        
    status = CmtGetLockEx (threadLock, 0, CMT_WAIT_FOREVER, &gotLock);
    if (status != 0)
    {
        MessagePopup ("Error", "Unable to get thread lock.");
        return -1;
    }
        
    // Check for block stop conditions
    for (ii=0; ii<block.num_low_points; ii++)
    {        
        // Active block reached the bottom
        if (block.low_points[ii].y == GRID_NUM_ROWS)
        {
            block_stop = 1;
            break;
        }
        
        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.low_points[ii].x, block.low_points[ii].y+1), 
                               ATTR_TEXT_BGCOLOR, &color);
        
        // Active block reached another block
        if (color != VAL_WHITE)
        {
            block_stop = 1;
            break;
        }
    }
    
    // Stop the active block
    if (block_stop == 1)
    {
        // Turn off timer
        SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_ENABLED, 0);
        
        CheckForLineClears ();
        
        // Spawn a new block       
        game_status = SpawnBlock (FIRST_BLOCK_NO);
        
        if (game_status == GAME_END)
        {
            // Dim grid
            SetCtrlAttribute (main_ph, PNLMAIN_GRID, ATTR_DIMMED, 1);
              
            // Show game over text
            SetCtrlAttribute (main_ph, PNLMAIN_TEXTGAMEOVER, ATTR_VISIBLE, 1);
            
            PlaySound (SFX_GAME_OVER, NULL, SND_FILENAME | SND_ASYNC);
          
            // Dim buttons
            SetCtrlAttribute (main_ph, PNLMAIN_BTNPAUSE, ATTR_DIMMED, 1);
            SetCtrlAttribute (main_ph, PNLMAIN_BTNROTATECCW, ATTR_DIMMED, 1);
            SetCtrlAttribute (main_ph, PNLMAIN_BTNROTATECW, ATTR_DIMMED, 1);
            SetCtrlAttribute (main_ph, PNLMAIN_BTNLEFT, ATTR_DIMMED, 1);
            SetCtrlAttribute (main_ph, PNLMAIN_BTNRIGHT, ATTR_DIMMED, 1);
            SetCtrlAttribute (main_ph, PNLMAIN_BTNDOWN, ATTR_DIMMED, 1);
        }
        else
        {        
            // Turn on timer
            SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_ENABLED, 1);
        }
    }
    // Move the active block
    else
    {     
        // Clear current position
        for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
        {                     
            SetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, VAL_WHITE);            
        }           
        
        // Move each square down 1 row                
        for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
        {                                          
            block.position[ii].y += 1;
            block.low_points[ii].y += 1;
            block.rows[ii] += 1;
            block.left_points[ii].y += 1;
            block.right_points[ii].y += 1;
            SetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, block.color);                    
        }              
    }
    
    status = CmtReleaseLock (threadLock);
    if (status != 0)
    {
        MessagePopup ("Error", "Unable to release thread lock.");
        return -1;
    }    
    
    return 0;    

}  // End of AdvanceBlock()


int CVICALLBACK CB_BtnMoveDown (int panel, int control, int event,
                                void *callbackData, int eventData1, int eventData2)
{
    switch (event)
    {
        case EVENT_LEFT_CLICK:
                        
            // Increase block speed
            SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_INTERVAL, SOFTDROP_SPEED);
            
            SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, "Mouse DOWN\n");
          
            break;
            
        case EVENT_LEFT_CLICK_UP:
        
            // Revert block speed
            SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_INTERVAL, g_speed);
            
            SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, "Mouse UP\n");

            break;            
    }
    return 0;
    
}  // End of CB_BtnMoveDown()


int CVICALLBACK CB_BtnMoveLeft (int panel, int control, int event,
                                void *callbackData, int eventData1, int eventData2)
{
    int colors[NUM_SQUARES_PER_BLOCK] = { VAL_WHITE, VAL_WHITE, VAL_WHITE, VAL_WHITE };
    int gotLock = 0;
    int ii = 0;  // Loop iterator
    int jj = 0;  // Loop iterator
    int matchFound = 0;
    int moveBlock = 1;
    int new_xvals[NUM_SQUARES_PER_BLOCK] = {0};
    int new_yvals[NUM_SQUARES_PER_BLOCK] = {0};
    int status = 0;
    
    switch (event)
    {
        case EVENT_COMMIT:
                        
            // Stop movement at the edge of the grid                 
            if ((block.position[0].x == 1) ||
                (block.position[1].x == 1) ||
                (block.position[2].x == 1) ||
                (block.position[3].x == 1))
            {
                break;
            }
                       
            // Check for clearances next to other blocks
            for (ii=0; ii<block.num_left_points; ii++)
            {
                GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.left_points[ii].x-1, block.left_points[ii].y), 
                                       ATTR_TEXT_BGCOLOR, &colors[ii]);                 
                if (colors[ii] != VAL_WHITE)
                {
                    moveBlock = 0;
                    break;
                }
            }
            
            if (moveBlock == 0)
            {
              break;
            }
            
            status = CmtGetLockEx (threadLock, 0, CMT_WAIT_FOREVER, &gotLock);
            if (status != 0)
            {
                MessagePopup ("Error", "Unable to get thread lock.");
                return -1;
            }  
                       
            // Get new block positions
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {                                        
                new_xvals[ii] = block.position[ii].x-1;
                new_yvals[ii] = block.position[ii].y;
                block.position[ii] = MakePoint (new_xvals[ii], block.position[ii].y);
            } 
            
            // Blank squares
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {     
                matchFound = 0;
                
                // Avoid blanking squares that need coloring
                for (jj=0; jj<NUM_SQUARES_PER_BLOCK; jj++)
                {
                    if (block.position[ii].x+1 == new_xvals[jj] &&
                        block.position[ii].y == new_yvals[jj])
                    {
                        matchFound = 1;
                    }
                }                            
                
                if (matchFound == 0)
                {
                    SetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[ii].x+1, block.position[ii].y), 
                                           ATTR_TEXT_BGCOLOR, VAL_WHITE);
                }
            } 
            
            // Color squares
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {     
                SetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, block.color);
            }
            
            for (ii=0; ii<block.num_low_points; ii++)
            {
                block.low_points[ii] = MakePoint (block.low_points[ii].x-1, block.low_points[ii].y);
            }
            
            for (ii=0; ii<block.num_left_points; ii++)
            {
                block.left_points[ii] = MakePoint (block.left_points[ii].x-1, block.left_points[ii].y);
                block.right_points[ii] = MakePoint (block.right_points[ii].x-1, block.right_points[ii].y);
            }
            
            status = CmtReleaseLock (threadLock);
            if (status != 0)
            {
                MessagePopup ("Error", "Unable to release thread lock.");
                return -1;
            }    

            break;
    }
    return 0;
    
}  // End of CB_BtnMoveLeft()


int CVICALLBACK CB_BtnMoveRight (int panel, int control, int event,
                                 void *callbackData, int eventData1, int eventData2)
{
    int colors[NUM_SQUARES_PER_BLOCK] = { VAL_WHITE, VAL_WHITE, VAL_WHITE, VAL_WHITE };
    int gotLock = 0;
    int ii = 0;  // Loop iterator
    int jj = 0;  // Loop iterator
    int matchFound = 0;
    int moveBlock = 1;
    int new_xvals[NUM_SQUARES_PER_BLOCK] = {0};
    int new_yvals[NUM_SQUARES_PER_BLOCK] = {0};
    int status = 0;
    
    switch (event)
    {
        case EVENT_COMMIT:            
            
            // Stop movement at the edge of the grid            
            if ((block.position[0].x == GRID_NUM_COLS) ||
                (block.position[1].x == GRID_NUM_COLS) ||
                (block.position[2].x == GRID_NUM_COLS) ||
                (block.position[3].x == GRID_NUM_COLS))
            {
                break;
            }
            
            // Check for clearances next to other blocks
            for (ii=0; ii<block.num_right_points; ii++)
            {
                GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.right_points[ii].x+1, block.right_points[ii].y), 
                                       ATTR_TEXT_BGCOLOR, &colors[ii]);                 
                if (colors[ii] != VAL_WHITE)
                {
                    moveBlock = 0;
                    break;
                }
            }
            
            if (moveBlock == 0)
            {
              break;
            }
                
            status = CmtGetLockEx (threadLock, 0, CMT_WAIT_FOREVER, &gotLock);
            if (status != 0)
            {
                MessagePopup ("Error", "Unable to get thread lock.");
                return -1;
            }            
                       
            // Get new block positions
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {                                        
                new_xvals[ii] = block.position[ii].x+1;
                new_yvals[ii] = block.position[ii].y;
                block.position[ii] = MakePoint (new_xvals[ii], block.position[ii].y);
            } 
            
            // Blank squares
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {     
                matchFound = 0;
                
                // Avoid blanking squares that need coloring
                for (jj=0; jj<NUM_SQUARES_PER_BLOCK; jj++)
                {
                    if (block.position[ii].x-1 == new_xvals[jj] &&
                        block.position[ii].y == new_yvals[jj])
                    {
                        matchFound = 1;
                    }
                }                            
                
                if (matchFound == 0)
                {
                    SetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[ii].x-1, block.position[ii].y), 
                                           ATTR_TEXT_BGCOLOR, VAL_WHITE);
                }
            } 
            
            // Color squares
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {     
                SetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, block.color);
            }            
            
            for (ii=0; ii<block.num_low_points; ii++)
            {
                block.low_points[ii] = MakePoint (block.low_points[ii].x+1, block.low_points[ii].y);
            }
            
            for (ii=0; ii<block.num_left_points; ii++)
            {
                block.left_points[ii] = MakePoint (block.left_points[ii].x+1, block.left_points[ii].y);
                block.right_points[ii] = MakePoint (block.right_points[ii].x+1, block.right_points[ii].y);
            }
            
            status = CmtReleaseLock (threadLock);
            if (status != 0)
            {
                MessagePopup ("Error", "Unable to release thread lock.");
                return -1;
            }    

            break;
    }
    return 0;
    
}  // End of CB_BtnMoveRight()


int CVICALLBACK CB_BtnPause (int panel, int control, int event,
                             void *callbackData, int eventData1, int eventData2)
{
    static int paused = 0;
    
    switch (event)
    {
        case EVENT_COMMIT:
            
            if (paused == 0)
            {
                SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_ENABLED, 0);
                
                // Dim block control buttons
                SetCtrlAttribute (main_ph, PNLMAIN_BTNROTATECCW, ATTR_DIMMED, 1);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNROTATECW, ATTR_DIMMED, 1);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNLEFT, ATTR_DIMMED, 1);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNRIGHT, ATTR_DIMMED, 1);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNSTART, ATTR_DIMMED, 1);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNDOWN, ATTR_DIMMED, 1);
                
                // Disable keyboard controls except for pause and quit
                SetMsgCallbackAttribute (main_ph, WM_KEYDOWN, ATTR_ENABLED, 0);
                SetMsgCallbackAttribute (main_ph, WM_KEYUP, ATTR_ENABLED, 0);

                paused = 1;
            }
            else
            {
                SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_ENABLED, 1);
                
                // Show block control buttons
                SetCtrlAttribute (main_ph, PNLMAIN_BTNROTATECCW, ATTR_DIMMED, 0);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNROTATECW, ATTR_DIMMED, 0);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNLEFT, ATTR_DIMMED, 0);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNRIGHT, ATTR_DIMMED, 0);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNSTART, ATTR_DIMMED, 0);
                SetCtrlAttribute (main_ph, PNLMAIN_BTNDOWN, ATTR_DIMMED, 0);
                
                // Enable keyboard controls
                SetMsgCallbackAttribute (main_ph, WM_KEYDOWN, ATTR_ENABLED, 1);
                SetMsgCallbackAttribute (main_ph, WM_KEYUP, ATTR_ENABLED, 1);
                paused = 0;
            }            
            break;
    }
    return 0;
    
}  // End of CB_BtnPause()


int CVICALLBACK CB_BtnQuit (int panel, int control, int event,
                            void *callbackData, int eventData1, int eventData2)
{
    switch (event)
    {
        case EVENT_COMMIT:
            SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_ENABLED, 0);
            QuitUserInterface (0);
            CmtDiscardLock (threadLock);
            break;
    }
    return 0;
    
}  // End of CB_BtnQuit()




int CVICALLBACK CB_BtnRotate (int panel, int control, int event,
                              void *callbackData, int eventData1, int eventData2)
{   
    switch (event)
    {
        case EVENT_COMMIT:            
                       
            // Handle single rotation blocks
            if ((block.type == BLOCK_I) ||
                (block.type == BLOCK_O) ||
                (block.type == BLOCK_S) ||
                (block.type == BLOCK_Z) ||
                (control == PNLMAIN_BTNROTATECW))
            {
                CB_BtnRotateCW (panel, control, EVENT_COMMIT, 0, 0, 0);
            }
            else
            {
                CB_BtnRotateCCW (panel, control, EVENT_COMMIT, 0, 0, 0);               
            }
                                    
            break;
    }
    return 0;
    
}  // End of CB_BtnRotate()
            

int CVICALLBACK CB_BtnRotateCCW (int panel, int control, int event,
                                 void *callbackData, int eventData1, int eventData2)
{
    char msg[512] = "\0";
    int colors[NUM_SQUARES_PER_BLOCK] = { VAL_WHITE, VAL_WHITE, VAL_WHITE, VAL_WHITE };
    int gotLock = 0;
    int ii = 0;  // Loop iterator
    int status = 0;
    
    switch (event)
    {
        case EVENT_COMMIT:
            
            status = CmtGetLockEx (threadLock, 0, CMT_WAIT_FOREVER, &gotLock);
            if (status != 0)
            {
                MessagePopup ("Error", "Unable to get thread lock.");
                return -1;
            }            
                       
            // Clear the block colors
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {                         
                SetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, VAL_WHITE);
            }  
            
            // Assign block color and coordinates
            switch (block.type)
            {                                                
                case BLOCK_J:
                                                                            
                    if (block.orientation == ORIENTATION_1)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x, block.position[2].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x, block.position[2].y-2), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_4;
                        block.position[0] = MakePoint (block.position[0].x+2, block.position[0].y-2);
                        block.position[1] = MakePoint (block.position[1].x+2, block.position[1].y);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[3];
                        block.low_points[1] = block.position[2];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[1].y;
                        block.rows[2] = block.position[2].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.left_points[2] = block.position[3];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[1];
                        block.right_points[2] = block.position[2];
                    } 
                    else if (block.orientation == ORIENTATION_2)
                    {
                        // Check for grid edge clearances
                        if (block.position[3].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x+1, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-1, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-1, block.position[3].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }                      
                        
                        block.orientation = ORIENTATION_1;                        
                        block.position[0] = MakePoint (block.position[0].x-2, block.position[0].y+2);
                        block.position[1] = MakePoint (block.position[1].x-1, block.position[1].y);
                        block.position[2] = MakePoint (block.position[2].x+1, block.position[2].y+2);
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[0];
                        block.low_points[1] = block.position[3];
                        block.low_points[2] = block.position[2];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[1].y;
                        block.rows[1] = block.position[0].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[1];
                        block.left_points[1] = block.position[0];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[1];
                        block.right_points[1] = block.position[2];
                    }
                    else if (block.orientation == ORIENTATION_3)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x, block.position[1].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x, block.position[1].y+1), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x+1, block.position[1].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }                        
                        
                        block.orientation = ORIENTATION_2;                        
                        block.position[0] = MakePoint (block.position[0].x, block.position[0].y-2);
                        // Note that block.position[1] does not change during rotation
                        block.position[2] = MakePoint (block.position[2].x-1, block.position[2].y-1);                       
                        block.position[3] = MakePoint (block.position[3].x+1, block.position[3].y+1);
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[3];
                        block.low_points[1] = block.position[0];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[2].y;
                        block.rows[1] = block.position[1].y;
                        block.rows[2] = block.position[3].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[2];
                        block.left_points[1] = block.position[1];
                        block.left_points[2] = block.position[3];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[1];
                        block.right_points[2] = block.position[3];
                    }                                
                    else if (block.orientation == ORIENTATION_4)
                    {
                        // Check for grid edge clearances
                        if (block.position[3].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x-1, block.position[1].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x-2, block.position[1].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_3;
                        block.position[0] = MakePoint (block.position[0].x, block.position[0].y+2);
                        block.position[1] = MakePoint (block.position[1].x-1, block.position[1].y);
                        block.position[2] = MakePoint (block.position[2].x, block.position[2].y-1);
                        block.position[3] = MakePoint (block.position[3].x-1, block.position[3].y-1);
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[3];
                        block.low_points[1] = block.position[1];
                        block.low_points[2] = block.position[0];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[2].y;
                        block.rows[1] = block.position[0].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[3];
                        block.left_points[1] = block.position[0];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[2];
                        block.right_points[1] = block.position[0];
                    }
                    break; 
                    
                case BLOCK_L:
                    
                    if (block.orientation == ORIENTATION_1)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x, block.position[3].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-1, block.position[3].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_4;
                        block.position[0] = MakePoint (block.position[0].x+1, block.position[0].y-2);
                        block.position[1] = MakePoint (block.position[1].x+1, block.position[1].y-2);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[2];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[3].y;
                        block.rows[2] = block.position[2].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[1];
                        block.left_points[1] = block.position[3];
                        block.left_points[2] = block.position[2];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[3];
                        block.right_points[2] = block.position[2];
                    }
                    else if (block.orientation == ORIENTATION_2)
                    {
                        // Check for grid edge clearances
                        if (block.position[1].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x-1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x, block.position[3].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }     
                        
                        block.orientation = ORIENTATION_1;
                        block.position[0] = MakePoint (block.position[0].x, block.position[0].y+2);
                        block.position[1] = MakePoint (block.position[1].x-1, block.position[1].y+1);
                        block.position[2] = MakePoint (block.position[2].x+1, block.position[2].y);
                        block.position[3] = MakePoint (block.position[3].x, block.position[3].y-1);
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[0];
                        block.low_points[2] = block.position[2];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[3].y;
                        block.rows[1] = block.position[2].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[3];
                        block.left_points[1] = block.position[1];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[3];
                        block.right_points[1] = block.position[2];
                    }
                    else if (block.orientation == ORIENTATION_3)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x, block.position[1].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x, block.position[1].y+1), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x+1, block.position[1].y+1), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_2;
                        block.position[0] = MakePoint (block.position[0].x+1, block.position[0].y-1);
                        // Note that block.position[1] does not change during rotation
                        block.position[2] = MakePoint (block.position[2].x+1, block.position[2].y);
                        block.position[3] = MakePoint (block.position[3].x, block.position[3].y+1);
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[2];
                        block.low_points[1] = block.position[3];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[1].y;
                        block.rows[2] = block.position[2].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.left_points[2] = block.position[2];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[1];
                        block.right_points[2] = block.position[3];
                    }
                    else if (block.orientation == ORIENTATION_4)
                    {
                        // Check for grid edge clearances
                        if (block.position[2].x == 2)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-1, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-2, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-2, block.position[3].y+1), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }                            
                        
                        block.orientation = ORIENTATION_3;
                        block.position[0] = MakePoint (block.position[0].x-2, block.position[0].y+1);
                        block.position[1] = MakePoint (block.position[1].x, block.position[1].y+1);
                        block.position[2] = MakePoint (block.position[2].x-2, block.position[2].y);
                        // Note that block.position[3] does not change during rotation                        
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[2];
                        block.low_points[1] = block.position[1];
                        block.low_points[2] = block.position[3];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[2].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[2];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[3];
                        block.right_points[1] = block.position[2];
                    }                    
                    break;
                                                    
                case BLOCK_T:
                    
                    if (block.orientation == ORIENTATION_1)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x, block.position[2].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        if (colors[0] != VAL_WHITE)
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_4;
                        block.num_rows = 3;
                        block.position[0] = MakePoint (block.position[0].x-1, block.position[0].y+1);
                        block.position[1] = MakePoint (block.position[1].x+1, block.position[1].y+1);
                        // Note that block.position[2] does not change during rotation
                        block.position[3] = MakePoint (block.position[3].x-1, block.position[3].y-1); 
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[0];
                        block.low_points[1] = block.position[1];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[3].y;
                        block.rows[1] = block.position[2].y;
                        block.rows[2] = block.position[1].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[3];
                        block.left_points[1] = block.position[0];
                        block.left_points[2] = block.position[1];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[3];
                        block.right_points[1] = block.position[2];
                        block.right_points[2] = block.position[1];
                    }
                    else if (block.orientation == ORIENTATION_2)
                    {
                        // Check for grid edge clearances
                        if (block.position[2].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x-1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        if (colors[0] != VAL_WHITE)
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_1;
                        // Note that block.position[0] does not change during rotation
                        block.position[1] = MakePoint (block.position[1].x-1, block.position[1].y-1);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[2];
                        block.low_points[2] = block.position[3];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[2].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[3];
                    }
                    else if (block.orientation == ORIENTATION_3)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x, block.position[2].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        if (colors[0] != VAL_WHITE)
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_2;
                        block.position[0] = MakePoint (block.position[0].x+1, block.position[0].y-1);
                        // Note that block.position[1] does not change during rotation                        
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation 
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[3];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[2].y;
                        block.rows[2] = block.position[1].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[2];
                        block.left_points[2] = block.position[1];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[3];
                        block.right_points[2] = block.position[1];
                    }
                    else if (block.orientation == ORIENTATION_4)
                    {
                        // Check for grid edge clearances
                        if (block.position[2].x == GRID_NUM_COLS)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x+1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        if (colors[0] != VAL_WHITE)
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_3;                        
                        // Note that block.position[0] does not change during rotation
                        // Note that block.position[1] does not change during rotation
                        // Note that block.position[2] does not change during rotation
                        block.position[3] = MakePoint (block.position[3].x+1, block.position[3].y+1);
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[0];
                        block.low_points[1] = block.position[1];
                        block.low_points[2] = block.position[3];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[2].y;
                        block.rows[1] = block.position[1].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[3];
                        block.right_points[1] = block.position[1];
                    }                    
                    break;                                                                    
                                                                        
                default:
                    sprintf (msg, "Unknown block type: %d\n", block.type);
                    SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, msg);
                    break;
                                                                            
            }  // End of switch()
                    
            // Set the block colors
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {                         
                SetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, block.color);
            }     
            
            status = CmtReleaseLock (threadLock);
            if (status != 0)
            {
                MessagePopup ("Error", "Unable to release thread lock.");
                return -1;
            }       
            
            PlaySound (SFX_ROTATE, NULL, SND_FILENAME | SND_ASYNC); 

            break;
    }
    return 0;
    
}  // End of CB_BtnRotateCCW()


int CVICALLBACK CB_BtnRotateCW (int panel, int control, int event,
                                void *callbackData, int eventData1, int eventData2)
{
    char msg[512] = "\0";
    int colors[NUM_SQUARES_PER_BLOCK] = { VAL_WHITE, VAL_WHITE, VAL_WHITE, VAL_WHITE };
    int gotLock = 0;
    int ii = 0;  // Loop iterator
    int status = 0;
    
    switch (event)
    {
        case EVENT_COMMIT:
            
            status = CmtGetLockEx (threadLock, 0, CMT_WAIT_FOREVER, &gotLock);
            if (status != 0)
            {
                MessagePopup ("Error", "Unable to get thread lock.");
                return -1;
            }            
                       
            // Clear the block colors
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {                         
                SetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, VAL_WHITE);
            }  
            
            // Assign block color and coordinates
            switch (block.type)
            {
                case BLOCK_I:
                    
                    if (block.orientation == ORIENTATION_1)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x, block.position[3].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x, block.position[3].y-2), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x, block.position[3].y-3), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }                        
                                                
                        block.orientation = ORIENTATION_2;                    
                        block.position[0] = MakePoint (block.position[0].x-1, block.position[0].y-3);
                        block.position[1] = MakePoint (block.position[1].x+2, block.position[1].y-2);
                        block.position[2] = MakePoint (block.position[2].x+1, block.position[2].y-1);
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 1;
                        block.low_points[0] = block.position[3];                        
                        block.num_rows = 4;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[1].y;
                        block.rows[2] = block.position[2].y;
                        block.rows[3] = block.position[3].y;
                        block.num_left_points = 4;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.left_points[2] = block.position[2];
                        block.left_points[3] = block.position[3];
                        block.num_right_points = 4;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[1];
                        block.right_points[2] = block.position[2];
                        block.right_points[3] = block.position[3];
                    }
                    else if (block.orientation == ORIENTATION_2)
                    {
                        // Check for grid edge clearances
                        if ((block.position[3].x < 3) ||
                            (block.position[3].x == GRID_NUM_COLS))
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-2, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-1, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x+1, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }
                                                                                            
                        block.orientation = ORIENTATION_1;
                        block.position[0] = MakePoint (block.position[0].x+1, block.position[0].y+3);
                        block.position[1] = MakePoint (block.position[1].x-2, block.position[1].y+2);
                        block.position[2] = MakePoint (block.position[2].x-1, block.position[2].y+1);
                        // Note that block.position[3] does not change during rotation                                            
                        block.num_low_points = 4;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[2];
                        block.low_points[2] = block.position[3];
                        block.low_points[3] = block.position[0];
                        block.num_rows = 1;
                        block.rows[0] = block.position[1].y;
                        block.num_left_points = 1;
                        block.left_points[0] = block.position[1];
                        block.num_right_points = 1;
                        block.right_points[0] = block.position[0];                        
                    }
                    break;
                                        
                case BLOCK_J:
                                                        
                    if (block.orientation == ORIENTATION_1)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x, block.position[3].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x, block.position[3].y-2), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x+1, block.position[3].y-2), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }                        
                        
                        block.orientation = ORIENTATION_2;                        
                        block.position[0] = MakePoint (block.position[0].x+2, block.position[0].y-2);
                        block.position[1] = MakePoint (block.position[1].x+1, block.position[1].y);                       
                        block.position[2] = MakePoint (block.position[2].x-1, block.position[2].y-2);
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[3];
                        block.low_points[1] = block.position[0];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[2].y;
                        block.rows[1] = block.position[1].y;
                        block.rows[2] = block.position[3].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[2];
                        block.left_points[1] = block.position[1];
                        block.left_points[2] = block.position[3];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[1];
                        block.right_points[2] = block.position[3];
                    }                                
                    else if (block.orientation == ORIENTATION_2)
                    {
                        // Check for grid edge clearances
                        if (block.position[3].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x-1, block.position[1].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x+1, block.position[1].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x+1, block.position[1].y+1), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_3;
                        block.position[0] = MakePoint (block.position[0].x, block.position[0].y+2);
                        // Note that block.position[1] does not change during rotation
                        block.position[2] = MakePoint (block.position[2].x+1, block.position[2].y+1);
                        block.position[3] = MakePoint (block.position[3].x-1, block.position[3].y-1);
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[3];
                        block.low_points[1] = block.position[1];
                        block.low_points[2] = block.position[0];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[2].y;
                        block.rows[1] = block.position[0].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[3];
                        block.left_points[1] = block.position[0];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[2];
                        block.right_points[1] = block.position[0];
                    }
                    else if (block.orientation == ORIENTATION_3)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x, block.position[2].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[0].x-1, block.position[0].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_4;
                        block.position[0] = MakePoint (block.position[0].x, block.position[0].y-2);
                        block.position[1] = MakePoint (block.position[1].x+1, block.position[1].y);
                        block.position[2] = MakePoint (block.position[2].x, block.position[2].y+1);
                        block.position[3] = MakePoint (block.position[3].x+1, block.position[3].y+1);
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[3];
                        block.low_points[1] = block.position[2];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[1].y;
                        block.rows[2] = block.position[2].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.left_points[2] = block.position[3];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[1];
                        block.right_points[2] = block.position[2];
                    } 
                    else if (block.orientation == ORIENTATION_4)
                    {
                        // Check for grid edge clearances
                        if (block.position[3].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-1, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-1, block.position[3].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }                      
                        
                        block.orientation = ORIENTATION_1;                        
                        block.position[0] = MakePoint (block.position[0].x-2, block.position[0].y+2);
                        block.position[1] = MakePoint (block.position[1].x-2, block.position[1].y);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[0];
                        block.low_points[1] = block.position[3];
                        block.low_points[2] = block.position[2];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[1].y;
                        block.rows[1] = block.position[0].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[1];
                        block.left_points[1] = block.position[0];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[1];
                        block.right_points[1] = block.position[2];
                    }
                    break; 
                    
                case BLOCK_L:
                    
                    if (block.orientation == ORIENTATION_1)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[0].x, block.position[0].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[0].x, block.position[0].y-2), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_2;
                        block.position[0] = MakePoint (block.position[0].x, block.position[0].y-2);
                        block.position[1] = MakePoint (block.position[1].x+1, block.position[1].y-1);
                        block.position[2] = MakePoint (block.position[2].x-1, block.position[2].y);
                        block.position[3] = MakePoint (block.position[3].x, block.position[3].y+1);
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[2];
                        block.low_points[1] = block.position[3];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[1].y;
                        block.rows[2] = block.position[2].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.left_points[2] = block.position[2];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[1];
                        block.right_points[2] = block.position[3];
                    }
                    else if (block.orientation == ORIENTATION_2)
                    {
                        // Check for grid edge clearances
                        if (block.position[2].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x-1, block.position[1].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x+1, block.position[1].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[1].x-1, block.position[1].y+1), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }                            
                        
                        block.orientation = ORIENTATION_3;
                        block.position[0] = MakePoint (block.position[0].x-1, block.position[0].y+1);
                        // Note that block.position[1] does not change during rotation
                        block.position[2] = MakePoint (block.position[2].x-1, block.position[2].y);
                        block.position[3] = MakePoint (block.position[3].x, block.position[3].y-1);
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[2];
                        block.low_points[1] = block.position[1];
                        block.low_points[2] = block.position[3];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[2].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[2];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[3];
                        block.right_points[1] = block.position[2];
                    }
                    else if (block.orientation == ORIENTATION_3)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x, block.position[3].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x, block.position[3].y+1), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-1, block.position[3].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[2]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE) || (colors[2] != VAL_WHITE))
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_4;
                        block.position[0] = MakePoint (block.position[0].x+2, block.position[0].y-1);
                        block.position[1] = MakePoint (block.position[1].x, block.position[1].y-1);
                        block.position[2] = MakePoint (block.position[2].x+2, block.position[2].y);
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[2];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[3].y;
                        block.rows[2] = block.position[2].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[1];
                        block.left_points[1] = block.position[3];
                        block.left_points[2] = block.position[2];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[3];
                        block.right_points[2] = block.position[2];
                    }
                    else if (block.orientation == ORIENTATION_4)
                    {
                        // Check for grid edge clearances
                        if (block.position[1].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x-1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x-2, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }     
                        
                        block.orientation = ORIENTATION_1;
                        block.position[0] = MakePoint (block.position[0].x-1, block.position[0].y+2);
                        block.position[1] = MakePoint (block.position[1].x-1, block.position[1].y+2);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[0];
                        block.low_points[2] = block.position[2];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[3].y;
                        block.rows[1] = block.position[2].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[3];
                        block.left_points[1] = block.position[1];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[3];
                        block.right_points[1] = block.position[2];
                    }
                    break;
                    
                case BLOCK_O:
                    // Same orientation for every rotation
                    break;
                    
                case BLOCK_S:
                    
                    if (block.orientation == ORIENTATION_1)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x-1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x-1, block.position[2].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_2;
                        block.position[0] = MakePoint (block.position[0].x-2, block.position[0].y-1);
                        block.position[1] = MakePoint (block.position[1].x, block.position[1].y-1);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[3];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[1].y;
                        block.rows[2] = block.position[3].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.left_points[2] = block.position[3];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[2];
                        block.right_points[2] = block.position[3];
                    }
                    else if (block.orientation == ORIENTATION_2)
                    {
                        // Check for grid edge clearances
                        if (block.position[2].x == GRID_NUM_COLS)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x+1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x-1, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }  
                        
                        block.orientation = ORIENTATION_1;
                        block.position[0] = MakePoint (block.position[0].x+2, block.position[0].y+1);
                        block.position[1] = MakePoint (block.position[1].x, block.position[1].y+1);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[3];
                        block.low_points[2] = block.position[0];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[2].y;
                        block.rows[1] = block.position[3].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[2];
                        block.left_points[1] = block.position[1];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[3];
                    }                    
                    break;
                    
                case BLOCK_T:
                    
                    if (block.orientation == ORIENTATION_1)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x, block.position[2].y+1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        if (colors[0] != VAL_WHITE)
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_2;
                        // Note that block.position[0] does not change during rotation
                        block.position[1] = MakePoint (block.position[1].x+1, block.position[1].y+1);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation 
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[3];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[2].y;
                        block.rows[2] = block.position[1].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[2];
                        block.left_points[2] = block.position[1];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[3];
                        block.right_points[2] = block.position[1];
                    }
                    else if (block.orientation == ORIENTATION_2)
                    {
                        // Check for grid edge clearances
                        if (block.position[2].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x-1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        if (colors[0] != VAL_WHITE)
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_3;
                        block.position[0] = MakePoint (block.position[0].x-1, block.position[0].y+1);
                        // Note that block.position[1] does not change during rotation
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[0];
                        block.low_points[1] = block.position[1];
                        block.low_points[2] = block.position[3];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[2].y;
                        block.rows[1] = block.position[1].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[3];
                        block.right_points[1] = block.position[1];
                    }
                    else if (block.orientation == ORIENTATION_3)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x, block.position[2].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        if (colors[0] != VAL_WHITE)
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_4;
                        block.num_rows = 3;
                        // Note that block.position[0] does not change during rotation
                        // Note that block.position[1] does not change during rotation
                        // Note that block.position[2] does not change during rotation
                        block.position[3] = MakePoint (block.position[3].x-1, block.position[3].y-1); 
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[0];
                        block.low_points[1] = block.position[1];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[3].y;
                        block.rows[1] = block.position[2].y;
                        block.rows[2] = block.position[1].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[3];
                        block.left_points[1] = block.position[0];
                        block.left_points[2] = block.position[1];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[3];
                        block.right_points[1] = block.position[2];
                        block.right_points[2] = block.position[1];
                    }
                    else if (block.orientation == ORIENTATION_4)
                    {
                        // Check for grid edge clearances
                        if (block.position[2].x == GRID_NUM_COLS)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x+1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        if (colors[0] != VAL_WHITE)
                        {
                            break;
                        }
                        
                        block.orientation = ORIENTATION_1;
                        block.position[0] = MakePoint (block.position[0].x+1, block.position[0].y-1);
                        block.position[1] = MakePoint (block.position[1].x-1, block.position[1].y-1);
                        // Note that block.position[2] does not change during rotation
                        block.position[3] = MakePoint (block.position[3].x+1, block.position[3].y+1); 
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[1];
                        block.low_points[1] = block.position[2];
                        block.low_points[2] = block.position[3];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[2].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[1];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[3];
                    }
                    break;                    
                    
                case BLOCK_Z:
                    
                    if (block.orientation == ORIENTATION_1)
                    {
                        // No need to check for grid edge clearances for this orientation
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x+1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x+1, block.position[2].y-1), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }  
                        
                        block.orientation = ORIENTATION_2;
                        block.position[0] = MakePoint (block.position[0].x+2, block.position[0].y-1);
                        block.position[1] = MakePoint (block.position[1].x, block.position[1].y-1);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 2;
                        block.low_points[0] = block.position[3];
                        block.low_points[1] = block.position[1];                        
                        block.num_rows = 3;
                        block.rows[0] = block.position[0].y;
                        block.rows[1] = block.position[1].y;
                        block.rows[2] = block.position[3].y;
                        block.num_left_points = 3;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[2];
                        block.left_points[2] = block.position[3];
                        block.num_right_points = 3;
                        block.right_points[0] = block.position[0];
                        block.right_points[1] = block.position[1];
                        block.right_points[2] = block.position[3];
                    }
                    else if (block.orientation == ORIENTATION_2)
                    {
                        // Check for grid edge clearances
                        if (block.position[2].x == 1)
                        {
                            break;
                        }
                        
                        // Check for clearances around other blocks
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[2].x-1, block.position[2].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[0]); 
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (block.position[3].x+1, block.position[3].y), 
                                               ATTR_TEXT_BGCOLOR, &colors[1]);
                        if ((colors[0] != VAL_WHITE) || (colors[1] != VAL_WHITE))
                        {
                            break;
                        }  
                        
                        block.orientation = ORIENTATION_1;
                        block.position[0] = MakePoint (block.position[0].x-2, block.position[0].y+1);
                        block.position[1] = MakePoint (block.position[1].x, block.position[1].y+1);
                        // Note that block.position[2] does not change during rotation
                        // Note that block.position[3] does not change during rotation
                        block.num_low_points = 3;
                        block.low_points[0] = block.position[0];
                        block.low_points[1] = block.position[3];
                        block.low_points[2] = block.position[1];                        
                        block.num_rows = 2;
                        block.rows[0] = block.position[2].y;
                        block.rows[1] = block.position[3].y;
                        block.num_left_points = 2;
                        block.left_points[0] = block.position[0];
                        block.left_points[1] = block.position[3];
                        block.num_right_points = 2;
                        block.right_points[0] = block.position[2];
                        block.right_points[1] = block.position[1];
                    }            
                    break;                    
                                                                        
                default:
                    sprintf (msg, "Unknown block type: %d\n", block.type);
                    SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, msg);
                    break;
                                                                            
            }  // End of switch()
                    
            // Set the block colors
            for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
            {                         
                SetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, block.color);
            }     
            
            status = CmtReleaseLock (threadLock);
            if (status != 0)
            {
                MessagePopup ("Error", "Unable to release thread lock.");
                return -1;
            }       
            
            PlaySound (SFX_ROTATE, NULL, SND_FILENAME | SND_ASYNC); 

            break;
    }
    return 0;
    
}  // End of CB_BtnRotateCW


int CVICALLBACK CB_BtnStart (int panel, int control, int event,
                             void *callbackData, int eventData1, int eventData2)
{    
    switch (event)
    {
        case EVENT_COMMIT:

            ClearGrid ();     
            
            // Clear log
            DeleteTextBoxLines (main_ph, PNLMAIN_TEXTLOG, 0, -1);
            
            // Undim grid
            SetCtrlAttribute (main_ph, PNLMAIN_GRID, ATTR_DIMMED, 0);
              
            // Hide game over text
            SetCtrlAttribute (main_ph, PNLMAIN_TEXTGAMEOVER, ATTR_VISIBLE, 0);     
            
            // Show block control buttons
            SetCtrlAttribute (main_ph, PNLMAIN_BTNPAUSE, ATTR_DIMMED, 0); 
            SetCtrlAttribute (main_ph, PNLMAIN_BTNROTATECCW, ATTR_DIMMED, 0);
            SetCtrlAttribute (main_ph, PNLMAIN_BTNROTATECW, ATTR_DIMMED, 0);
            SetCtrlAttribute (main_ph, PNLMAIN_BTNLEFT, ATTR_DIMMED, 0);
            SetCtrlAttribute (main_ph, PNLMAIN_BTNRIGHT, ATTR_DIMMED, 0);
            SetCtrlAttribute (main_ph, PNLMAIN_BTNDOWN, ATTR_DIMMED, 0);
            
            SetCtrlVal (main_ph, PNLMAIN_LEVEL, 1);
            SetCtrlVal (main_ph, PNLMAIN_NUMCLEARED, 0);
            
            SpawnBlock (FIRST_BLOCK_YES);
            
            // Set drop speed
            g_speed = NORMAL_SPEED;
            SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_INTERVAL, g_speed);
            
            // Start advancing blocks
            SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_ENABLED, 1);
            
            SetActiveCtrl (main_ph, PNLMAIN_BTNPAUSE);

            break;
    }
    return 0;
    
}  // End of CB_BtnStart()


int CVICALLBACK CB_KeyDown (int panelHandle, int message, unsigned int* wParam, 
                            unsigned int* lParam, void* callbackData)
{
    // Monitor down-arrow key for press
    if ((g_keydown == 0) && (*wParam == VK_DOWN))
    {      
        // Increase block speed
        SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_INTERVAL, SOFTDROP_SPEED);
        
        SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, "VK_DOWN DOWN\n");

        g_keydown = 1;
    }   
    // Monitor for CCW rotation
    else if ((g_keydown == 0) && ((*wParam == 'Z') || (*wParam == VK_CONTROL)))
    {
        CallCtrlCallback (main_ph, PNLMAIN_BTNROTATECCW, EVENT_COMMIT, 0, 0, 0);            
    }
    // Monitor for CW rotation
    else if ((g_keydown == 0) && (*wParam == 'X'))
    {
        CallCtrlCallback (main_ph, PNLMAIN_BTNROTATECW, EVENT_COMMIT, 0, 0, 0);            
    }    
    
    return 0;
}  // End of CB_KeyDown()


int CVICALLBACK CB_KeyUp (int panelHandle, int message, unsigned int* wParam, 
                          unsigned int* lParam, void* callbackData)
{
    // Monitor down-arrow key for release
    if (*wParam == VK_DOWN)
    {        
        // Revert block speed
        SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_INTERVAL, g_speed);
        
        SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, "VK_DOWN UP\n");
        
        g_keydown = 0;
    }
    return 0;
}  // End of CB_KeyUp()


int CVICALLBACK CB_TimerAdvanceBlock (int panel, int control, int event,
                                      void *callbackData, int eventData1, int eventData2)
{
    switch (event)
    {
        case EVENT_TIMER_TICK:
            AdvanceBlock ();
            break;
    }
    
    return 0;
    
}  // End of CB_TimerAdvanceBlock()


int CheckForLineClears (void)
{
    char msg[512] = "\0";
    int clearList[NUM_SQUARES_PER_BLOCK] = {0};
    int color = VAL_WHITE;
    int foundEmptyCell = 0;
    int ii = 0;  // Loop iterator
    int jj = 0;  // Loop iterator
    int kk = 0;  // Loop iterator
    int level = 0;
    int numLineClearsTotal = 0;
    int numLineClears = 0;
    
    sprintf (msg, "Line check start\n");
    SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, msg);    
    
    // Begin loop through each row of the active block
    for (ii=0; ii<block.num_rows; ii++)
    {
        // Initialize search flag
        foundEmptyCell = 0;
        
        // Begin loop through all grid columns
        for (jj=1; jj<=GRID_NUM_COLS; jj++)
        {
            // Search for empty cells
            GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (jj, block.rows[ii]), 
                                   ATTR_TEXT_BGCOLOR, &color);                    
            if (color == VAL_WHITE)
            {
                // DEBUG
                sprintf (msg, "Row %d: white cell in column %d\n", block.rows[ii], jj);
                SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, msg);                
                
                // Line should not be cleared
                foundEmptyCell = 1;
                break;
            }   
            
        }  // End loop through all grid columns
        
        // Encountered a solid line
        if (foundEmptyCell == 0)
        {
            // Mark line for clearing
            clearList[numLineClears] = block.rows[ii];
            numLineClears++;
        }    
        
    }  // End loop through each row of the active block
    
    // Clear any marked lines
    if (numLineClears > 0)
    {
        if (numLineClears == 4)
        {            
            PlaySound (SFX_CLEAR_4LINES, NULL, SND_FILENAME | SND_ASYNC);     
        }
        else
        {
            PlaySound (SFX_CLEAR_LINE, NULL, SND_FILENAME | SND_ASYNC);
        }
        
        // Begin loop through all rows to be cleared
        for (ii=0; ii<numLineClears; ii++)
        {
            sprintf (msg, "Clear row: %d\n", clearList[ii]);
            SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, msg);
            
            // Begin loop through all rows to be dropped down
            for (jj=clearList[ii]; jj>0; jj--)
            {          
                // Clear the row
                SetTableCellRangeAttribute (main_ph, PNLMAIN_GRID, MakeRect (jj, 1, 1, GRID_NUM_COLS), 
                                            ATTR_TEXT_BGCOLOR, VAL_WHITE);
                
                // Begin loop through all columns
                for (kk=1; kk<=GRID_NUM_COLS; kk++)
                {                    
                    // Drop all blocks down one row
                    if (jj > 1)  // Nothing to drop on the top row
                    {
                        GetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (kk, jj-1), ATTR_TEXT_BGCOLOR, &color);                                
                        SetTableCellAttribute (main_ph, PNLMAIN_GRID, MakePoint (kk, jj), ATTR_TEXT_BGCOLOR, color);
                    }
                    
                }  // End loop through all columns
                
            }  // End loop through all rows to be dropped down
            
        }  // End loop through all rows to be cleared
    }
    
    // Update number of lines cleared
    GetCtrlVal (main_ph, PNLMAIN_NUMCLEARED, &numLineClearsTotal);    
    numLineClearsTotal += numLineClears;    
    SetCtrlVal (main_ph, PNLMAIN_NUMCLEARED, numLineClearsTotal);
    
    // Update level
    level = (numLineClearsTotal / 10) + 1;
    SetCtrlVal (main_ph, PNLMAIN_LEVEL, level);
    
    // Update block speed
    g_speed = NORMAL_SPEED - ((level - 1) * LEVEL_SPEEDUP);
    SetCtrlAttribute (main_ph, PNLMAIN_TIMERADVANCE, ATTR_INTERVAL, g_speed);
    
    sprintf (msg, "Line check done\n");
    SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, msg);
    
    return 0;
    
}  // End of CheckForLineClears()


int ClearGrid (void)
{
    // Set all cells to white color
    SetTableCellRangeAttribute (main_ph, PNLMAIN_GRID, MakeRect (1, 1, GRID_NUM_ROWS, GRID_NUM_COLS), 
                                ATTR_TEXT_BGCOLOR, VAL_WHITE);
        
    return 0;
    
}  // End of ClearGrid()


int SpawnBlock (int first_block)
{
    // Ensure S and Z blocks are at end of array
    char blocks[NUM_BLOCKS_TYPES] = {BLOCK_I, BLOCK_J, BLOCK_L, BLOCK_O,
                                     BLOCK_T, BLOCK_S, BLOCK_Z};
    
    char msg[512] = "\0";
  
    int block_index = -1;
    int color = VAL_WHITE;
    int game_status = GAME_RUN;
    int ii = 0;  // Loop iterator    
      
    // Random first block
    if ((first_block == FIRST_BLOCK_YES) && (g_startBlock == BLOCK_RANDOM))
    {   
        // Start pseudo randomly
        srand (time (NULL));  
        
        // Avoid S and Z blocks on the first block
        block_index = rand () % (NUM_BLOCKS_TYPES - 2);
    }
    // Selective first block, for debug purposes
    else if (first_block == FIRST_BLOCK_YES)
    {
        switch (g_startBlock)
        {
            case BLOCK_I:
                block_index = 0;
                break;       
            case BLOCK_J:
                block_index = 1;
                break;                   
            case BLOCK_L:
                block_index = 2;
                break;
            case BLOCK_O:
                block_index = 3;
                break;                
            case BLOCK_S:
                block_index = 5;
                break;          
            case BLOCK_T:
                block_index = 4;
                break;                 
            case BLOCK_Z:
                block_index = 6;
                break;                   
            default:
                MessagePopup ("Error", "Unknown start block");
                return -1; 
        }
    }
    // Random subsequent blocks
    else
    {
        // Generate a random block
        block_index = rand () % NUM_BLOCKS_TYPES;
    }
    block.type = blocks[block_index];

    //sprintf (msg, "Spawn block: %c\n", block.type);
    //SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, msg);
    
    // Assign block color and coordinates
    switch (block.type)
    {
        case BLOCK_I:
            block.color = VAL_CYAN;
            block.orientation = ORIENTATION_1;
            block.position[0] = MakePoint (7, 4);
            block.position[1] = MakePoint (4, 4);
            block.position[2] = MakePoint (5, 4);
            block.position[3] = MakePoint (6, 4); 
            block.num_low_points = 4;
            block.low_points[0] = block.position[1];
            block.low_points[1] = block.position[2];
            block.low_points[2] = block.position[3];
            block.low_points[3] = block.position[0];
            block.num_rows = 1;
            block.rows[0] = 4;
            block.num_left_points = 1;
            block.left_points[0] = block.position[1];
            block.num_right_points = 1;
            block.right_points[0] = block.position[0];
            break;
            
        case BLOCK_J:
            block.color = VAL_BLUE;
            block.orientation = ORIENTATION_1;            
            block.position[0] = MakePoint (4, 3);
            block.position[1] = MakePoint (4, 2);
            block.position[2] = MakePoint (6, 3);
            block.position[3] = MakePoint (5, 3);
            block.num_low_points = 3;
            block.low_points[0] = block.position[0];
            block.low_points[1] = block.position[3];
            block.low_points[2] = block.position[2]; 
            block.num_rows = 2;
            block.rows[0] = 2;
            block.rows[1] = 3;
            block.num_left_points = 2;
            block.left_points[0] = block.position[1];
            block.left_points[1] = block.position[0];
            block.num_right_points = 2;
            block.right_points[0] = block.position[1];
            block.right_points[1] = block.position[2];
            break;  
            
        case BLOCK_L:            
            block.color = MakeColor (255, 165, 0);  // Orange
            block.orientation = ORIENTATION_1;            
            block.position[0] = MakePoint (5, 3);
            block.position[1] = MakePoint (4, 3);
            block.position[2] = MakePoint (6, 3);
            block.position[3] = MakePoint (6, 2);
            block.num_low_points = 3;
            block.low_points[0] = block.position[1];
            block.low_points[1] = block.position[0];
            block.low_points[2] = block.position[2];
            block.num_rows = 2;
            block.rows[0] = 2;
            block.rows[1] = 3;
            block.num_left_points = 2;
            block.left_points[0] = block.position[3];
            block.left_points[1] = block.position[1];
            block.num_right_points = 2;
            block.right_points[0] = block.position[3];
            block.right_points[1] = block.position[2];
            break;
            
        case BLOCK_O:
            block.color = VAL_YELLOW;
            block.orientation = ORIENTATION_1;            
            block.position[0] = MakePoint (5, 1);
            block.position[1] = MakePoint (5, 2);
            block.position[2] = MakePoint (6, 1);
            block.position[3] = MakePoint (6, 2);
            block.num_low_points = 2;
            block.low_points[0] = block.position[1];
            block.low_points[1] = block.position[3];            
            block.num_rows = 2;
            block.rows[0] = 1;
            block.rows[1] = 2;
            block.num_left_points = 2;
            block.left_points[0] = block.position[0];
            block.left_points[1] = block.position[1];
            block.num_right_points = 2;
            block.right_points[0] = block.position[2];
            block.right_points[1] = block.position[3];
            break; 
            
        case BLOCK_S:
            block.color = VAL_GREEN;
            block.orientation = ORIENTATION_1;            
            block.position[0] = MakePoint (6, 2);
            block.position[1] = MakePoint (4, 3);
            block.position[2] = MakePoint (5, 2);
            block.position[3] = MakePoint (5, 3);
            block.num_low_points = 3;
            block.low_points[0] = block.position[1];
            block.low_points[1] = block.position[3];
            block.low_points[2] = block.position[0]; 
            block.num_rows = 2;
            block.rows[0] = 2;
            block.rows[1] = 3;            
            block.num_left_points = 2;
            block.left_points[0] = block.position[2];
            block.left_points[1] = block.position[1];
            block.num_right_points = 2;
            block.right_points[0] = block.position[0];
            block.right_points[1] = block.position[3];
            break;   
            
        case BLOCK_T:
            block.color = MakeColor (163, 44, 196);  // Purple
            block.orientation = ORIENTATION_1;            
            block.position[0] = MakePoint (5, 1);
            block.position[1] = MakePoint (4, 2);
            block.position[2] = MakePoint (5, 2);
            block.position[3] = MakePoint (6, 2);
            block.num_low_points = 3;
            block.low_points[0] = block.position[1];
            block.low_points[1] = block.position[2];
            block.low_points[2] = block.position[3];            
            block.num_rows = 2;
            block.rows[0] = 1;
            block.rows[1] = 2;
            block.num_left_points = 2;
            block.left_points[0] = block.position[0];
            block.left_points[1] = block.position[1];
            block.num_right_points = 2;
            block.right_points[0] = block.position[0];
            block.right_points[1] = block.position[3];
            break;
            
        case BLOCK_Z:
            block.color = VAL_RED;
            block.orientation = ORIENTATION_1;            
            block.position[0] = MakePoint (4, 2);
            block.position[1] = MakePoint (6, 3);
            block.position[2] = MakePoint (5, 2);
            block.position[3] = MakePoint (5, 3);
            block.num_low_points = 3;
            block.low_points[0] = block.position[0];
            block.low_points[1] = block.position[3];
            block.low_points[2] = block.position[1];
            block.num_rows = 2;
            block.rows[0] = 2;
            block.rows[1] = 3;
            block.num_left_points = 2;
            block.left_points[0] = block.position[0];
            block.left_points[1] = block.position[3];
            block.num_right_points = 2;
            block.right_points[0] = block.position[2];
            block.right_points[1] = block.position[1];
            break;              
            
        default:
            sprintf (msg, "Unknown block: %c\n", block.type);
            SetCtrlVal (main_ph, PNLMAIN_TEXTLOG, msg);
            break;
    }

    // Set the block colors
    for (ii=0; ii<NUM_SQUARES_PER_BLOCK; ii++)
    {                         
        // Check for block overlap
        GetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, &color);        
        if (color != VAL_WHITE)
        {
            game_status = GAME_END;
        }
                
        SetTableCellAttribute (main_ph, PNLMAIN_GRID, block.position[ii], ATTR_TEXT_BGCOLOR, block.color);
    }    
    
    return game_status;
    
}  // End of SpawnBlock()