//==============================================================================
// Copyright (C) John-Philip Taylor
// tyljoh010@myuct.ac.za
//
// This file is part of the EEE4084F Course
//
// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>
//
// This is an adaptition of The "Hello World" example avaiable from
// https://en.wikipedia.org/wiki/Message_Passing_Interface#Example_program
//==============================================================================

/** \mainpage Prac3 Main Page
 *
 * \section intro_sec Introduction
 *
 * The purpose of Prac3 is to learn some basics of MPI coding.
 *
 * Look under the Files tab above to see documentation for particular files
 * in this project that have Doxygen comments.
 */

//---------- STUDENT NUMBERS --------------------------------------------------
//
// Nicolas Reid - [RDXNIC008]
// Callum Tilbury - [TLBCAL002]
//
//-----------------------------------------------------------------------------

/* Note that Doxygen comments are used in this file. */
/** \file Prac3
 *  Prac3 - MPI Main Module
 *  The purpose of this prac is to get a basic introduction to using
 *  the MPI libraries for prallel or cluster-based programming.
 */

// Includes needed for the program
#include "Prac3.h"
#include <time.h>
/** This is the master node function, describing the operations
    that the master will be doing */
void Master () {
 //! <h3>Local vars</h3>
 // The above outputs a heading to doxygen function entry
 MPI_Status stat;    //! stat: Status of the MPI application

 // Read the input image
 if(!Input.Read("Data/small.jpg")){
  printf("Cannot read image\n");
  return;
 }

 // Allocated RAM for the output image
 if(!Output.Allocate(Input.Width, Input.Height, Input.Components)) return;

 // Allocate memory for variables to store the partitioned RGB components
 int height = Input.Height;
 int width = Input.Width;
 unsigned char reds[height][width];     // Red elements; to be sent to node #1
 unsigned char greens[height][width];       // Green elements; to be sent to node #1
 unsigned char blues[height][width];        // Blue elements; to be sent to node #1

 // Send dimention info to slaves
 int size[2] = {height, width};
 MPI_Send(size, 2, MPI_INT, 1, TAG, MPI_COMM_WORLD);
 MPI_Send(size, 2, MPI_INT, 2, TAG, MPI_COMM_WORLD);
 MPI_Send(size, 2, MPI_INT, 3, TAG, MPI_COMM_WORLD);
 // Receive acknowledgement that message was recieved
 unsigned char ack[1];
 MPI_Recv(ack,1,MPI_BYTE,1,TAG,MPI_COMM_WORLD,&stat);
 MPI_Recv(ack,1,MPI_BYTE,2,TAG,MPI_COMM_WORLD,&stat);
 MPI_Recv(ack,1,MPI_BYTE,3,TAG,MPI_COMM_WORLD,&stat);

 // Partitioning
 // Itterate through rows of the input image
 int i;
 for(int y = 0; y < Input.Height; y++){
  i = 0;
  // Run through RGB pixel elements in each row and separate the components
  for(int x = 0; x < Input.Width*Input.Components; x+=3){
   reds[y][i] = Input.Rows[y][x];
   greens[y][i] = Input.Rows[y][x+1];
   blues[y][i] = Input.Rows[y][x+2];
   i++;
  }
 }

 // Send partitioned RGB data to slaves 1, 2, 3 respectively
 MPI_Send(reds, height*width, MPI_BYTE, 1, TAG, MPI_COMM_WORLD);
 MPI_Send(greens, height*width, MPI_BYTE, 2, TAG, MPI_COMM_WORLD);
 MPI_Send(blues, height*width, MPI_BYTE, 3, TAG, MPI_COMM_WORLD);
 // Receive acknowledgement that message was recieved
 MPI_Recv(ack,1,MPI_BYTE,1,TAG,MPI_COMM_WORLD,&stat);
 MPI_Recv(ack,1,MPI_BYTE,2,TAG,MPI_COMM_WORLD,&stat);
 MPI_Recv(ack,1,MPI_BYTE,3,TAG,MPI_COMM_WORLD,&stat);

 // Set aside memory space for incoming (filtered) RGB data
 unsigned char redsF[height][width];
 unsigned char greensF[height][width];
 unsigned char bluesF[height][width];

 // Receive filtered data from slaves 1, 2, 3
 MPI_Recv(redsF, height*width, MPI_BYTE, 1, TAG, MPI_COMM_WORLD, &stat);
 MPI_Recv(greensF, height*width, MPI_BYTE, 2, TAG, MPI_COMM_WORLD, &stat);
 MPI_Recv(bluesF, height*width, MPI_BYTE, 3, TAG, MPI_COMM_WORLD, &stat);
 MPI_Send(ack,1,MPI_BYTE,1,TAG,MPI_COMM_WORLD);
 MPI_Send(ack,1,MPI_BYTE,2,TAG,MPI_COMM_WORLD);
 MPI_Send(ack,1,MPI_BYTE,3,TAG,MPI_COMM_WORLD);

 // Re-organise the RGB components into the output image rows
 for(int y = 0; y < Input.Height; y++){
  i = 0;
  for(int x = 0; x < Input.Width*Input.Components; x+=3){
   Output.Rows[y][x] = redsF[y][i];
   Output.Rows[y][x+1] = greensF[y][i];
   Output.Rows[y][x+2] = bluesF[y][i];
   i++;
  }
 }

 // Write results to the output image jpg file
 if(!Output.Write("Data/Output.jpg")){
  printf("Cannot write image\n");
  return;
 }

 printf("\nFinished. Result written to Output.jpg\n");

 //! <h3>Output</h3> The file Output.jpg will be created on success to save
 //! the processed output.
}
//----------------------------------------------------------------------END MASTER

/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID){
 char idstr[32];
 int size[2];
 unsigned char ack[1];
 ack[0] = 'a';      // Arbitrary acknowlage message?

 int windowSize = 3;        // Set window size

 MPI_Status stat;

 printf("Processor %d reporting for duty.\n", ID);

 // Blocking receive from rank 0 (master):
 // Recieve dimention info
 MPI_Recv(size, 2, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
 MPI_Send(ack,1,MPI_BYTE,0,TAG,MPI_COMM_WORLD);
 int height = size[0];
 int width = size[1];

 // Allocate memory for the incoming rgb data streams
 unsigned char rgbIn[height][width];

 // Recieve r/g/b input data
 MPI_Recv(rgbIn, height*width, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
 MPI_Send(ack,1,MPI_BYTE,0,TAG,MPI_COMM_WORLD);

 // Allocate memory for the outgoining rgb data streams and temporary window array
 unsigned char rgbOut[height][width];
 unsigned char window[windowSize*windowSize];

 int margin = round(windowSize/2);
 int w, wStartX, wEndX, wStartY, wEndY;

 // Median filter
 // Loop through all rows in image
 for(int y = 0; y < height; ++y){

  if(y < margin){       // Case: upper rows of pixel components within margin
   wStartY = 0; wEndY = y + margin;
  } else if(y > width-margin){      // Case: lower rows of components within margin
   wStartY = y - margin; wEndY = height-1;
  } else{       // Case: all rows within vertical margin boundaries
   wStartY = y - margin; wEndY = y + margin;
  }

  // Loop through each pixel component in a row y
  for(int x = 0; x < width; ++x){

   if(x < margin){      // Case: left most pixel components within margin
    wStartX = 0; wEndX = x + margin;
   } else if(x > width-margin){     // Case: right most pixel components within margin
    wStartX = x - margin; wEndX = width-1;
   } else{      // Case: all component elements which fall within margins
    wStartX = x - margin; wEndX = x + margin;
   }

   w = 0;       // 1D Window array counter
   // Populate window array with pixel components in windowSize-by-windowSize surrounding area
   for(int wy = wStartY; wy < wEndY; ++wy){
    for(int wx = wStartX; wx < wEndX; ++wx){
     window[w++] = rgbIn[wy][wx];
    }
   }

   std::sort(window, window + w);       // Sort window array elements

   rgbOut[y][x] = window[w/2];      // Write the median (middle positioned) element to the output
  }
 }

 // Send filtered results back to rank 0 (master):
 MPI_Send(rgbOut, height*width, MPI_BYTE, 0, TAG, MPI_COMM_WORLD);
 MPI_Recv(ack,1,MPI_BYTE,0,TAG,MPI_COMM_WORLD,&stat);

}
//---------------------------------------------------------------------END SLAVE

/** This is the entry point to the program. */
int main(int argc, char** argv){

 clock_t start, end;
 double cpu_time_used;

 // Start timing
 start = clock();

 int myid;

 // MPI programs start with MPI_Init
 MPI_Init(&argc, &argv);

 // find out how big the world is
 MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

 // and this processes' rank is
 MPI_Comm_rank(MPI_COMM_WORLD, &myid);

 // At this point, all programs are running equivalently, the rank
 // distinguishes the roles of the programs, with
 // rank 0 often used as the "master".
 if(myid == 0) Master();
 else          Slave (myid);

 // MPI programs end with MPI_Finalize
 MPI_Finalize();

 end = clock();
 cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
 if(myid == 0)
  printf("\nTime lapsed: %f seconds\n", cpu_time_used);

 return 0;
}
//------------------------------------------------------------------------------
