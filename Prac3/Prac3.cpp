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
 int width = Input.Width*Input.Components/3;
 unsigned char** reds = new unsigned char*[height];
 unsigned char** greens = new unsigned char*[height];
 unsigned char** blues = new unsigned char*[height];
 for(int i = 0; i < height; i++){
  reds[i] = new unsigned char[width];
  greens[i] = new unsigned char[width];
  blues[i] = new unsigned char[width];
 }

 // Itterate through rows of the input image
 int i;
 for(int y = 0; y < Input.Height; y++){
  // Run through RGB pixel elements in each row and separate the components
  i = 0;
  for(int x = 0; x < Input.Width*Input.Components; x+=3){
   reds[y][i] = Input.Rows[y][x];
   greens[y][i] = Input.Rows[y][x+1];
   blues[y][i] = Input.Rows[y][x+2];
   i++;
  }
 }
 printf("Partitioning complete.\n");

 // Send dimention info to slaves
 int size[2] = {height, width};
 MPI_Send(size, 2, MPI_INT, 1, TAG, MPI_COMM_WORLD);
 MPI_Send(size, 2, MPI_INT, 2, TAG, MPI_COMM_WORLD);
 MPI_Send(size, 2, MPI_INT, 3, TAG, MPI_COMM_WORLD);
 unsigned char* ack = new unsigned char[1];
 MPI_Recv(ack,1,MPI_BYTE,1,TAG,MPI_COMM_WORLD,&stat);
 MPI_Recv(ack,1,MPI_BYTE,2,TAG,MPI_COMM_WORLD,&stat);
 MPI_Recv(ack,1,MPI_BYTE,3,TAG,MPI_COMM_WORLD,&stat);
 printf("Sent dimention info.");

 // Send partitioned data to slaves 1, 2, 3
 MPI_Send(reds, height*width, MPI_BYTE, 1, TAG, MPI_COMM_WORLD);
 MPI_Send(greens, height*width, MPI_BYTE, 2, TAG, MPI_COMM_WORLD);
 MPI_Send(blues, height*width, MPI_BYTE, 3, TAG, MPI_COMM_WORLD);
 MPI_Recv(ack,1,MPI_BYTE,1,TAG,MPI_COMM_WORLD,&stat);
 MPI_Recv(ack,1,MPI_BYTE,2,TAG,MPI_COMM_WORLD,&stat);
 MPI_Recv(ack,1,MPI_BYTE,3,TAG,MPI_COMM_WORLD,&stat);
 printf("Sent rgb data.\n");

 // Receive filtered data from slaves 1, 2, 3
 MPI_Recv(reds, height*width, MPI_BYTE, 1, TAG, MPI_COMM_WORLD, &stat);
 MPI_Recv(greens, height*width, MPI_BYTE, 2, TAG, MPI_COMM_WORLD, &stat);
 MPI_Recv(blues, height*width, MPI_BYTE, 3, TAG, MPI_COMM_WORLD, &stat);
 MPI_Send(ack,1,MPI_BYTE,1,TAG,MPI_COMM_WORLD);
 MPI_Send(ack,1,MPI_BYTE,2,TAG,MPI_COMM_WORLD);
 MPI_Send(ack,1,MPI_BYTE,3,TAG,MPI_COMM_WORLD);

 // Re-organise the RGB components into the output image rows
 for(int y = 0; y < Input.Height; y++){
  i = 0;
  for(int x = 0; x < Input.Width*Input.Components; x+=3){
   Output.Rows[y][x] = reds[y][i];
   Output.Rows[y][x+1] = greens[y][i];
   Output.Rows[y][x+2] = blues[y][i];
   i++;
  }
 }

 // Write the output image
 if(!Output.Write("Data/Output.jpg")){
  printf("Cannot write image\n");
  return;
 }
 //! <h3>Output</h3> The file Output.jpg will be created on success to save
 //! the processed output.
}
//------------------------------------------------------------------------------

/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID){
 char idstr[32];
 int size[2];
 int windowSize = 30;
 unsigned char* ack = new unsigned char[1];
 ack[0] = 'a';

 MPI_Status stat;

 printf("Processor %d reporting for duty.\n", ID);

 // Bollking receive from rank 0 (master):
 // Recieve dimention info
 MPI_Recv(size, 2, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
 MPI_Send(ack,1,MPI_BYTE,0,TAG,MPI_COMM_WORLD);
 printf("%d: Recieved dimention info.\n", ID);
 int height = size[0];
 int width = size[1];

 // Allocate memory for the incoming rgb data streams
 unsigned char** rgbIn = new unsigned char*[height];
 for(int i = 0; i < height; i++)
  rgbIn[i] = new unsigned char[width];

 // Recieve r/g/b input data
 MPI_Recv(rgbIn, height*width, MPI_BYTE, 0, TAG, MPI_COMM_WORLD, &stat);
 MPI_Send(ack,1,MPI_BYTE,0,TAG,MPI_COMM_WORLD);
 printf("%d: Recieved rgb data.\n", ID);

 // Allocate memory for the outgoining rgb data streams
 unsigned char** rgbOut = new unsigned char*[height];
 for(int i = 0; i < height; i++)
  rgbOut[i] = new unsigned char[width];

 unsigned char* window = new unsigned char[windowSize*windowSize];

 int margin = round(windowSize/2);
 int w, wStartX, wEndX, wStartY, wEndY;

 bool checked = TRUE, checked2 = TRUE;
//////////////////////////////////////////////////////////////////
 printf("\n%d: Starting test\n", ID);
 for(int y = 0; y < height-1; y++){
  for(int x = 0; x < width-1; x++){
   printf("\n%d: Loopy\n", ID);
   rgbOut[y][x] = rgbIn[y][x];
   printf("\n%d: x = %d:\n", ID, x);
  }
 }
 printf("\n\n\n\nFINISHED SETTING\n\n\n\n\n\n");
//////////////////////////////////////////////////////////////////

/* printf("\nStarting filter.\n");
 for(int y = 0; y < margin; ++y){

  if(y < margin){
   wStartY = 0; wEndY = y + margin;
  } else if(y > width-margin){
   wStartY = y - margin; wEndY = height-1;
  } else{
   wStartY = y - margin; wEndY = y + margin;
  }

  if(checked){
   printf("\nfirst loop\n");
   checked = FALSE;
  }

  for(int x = 0; x < width; ++x){

   if(x < margin){
    wStartX = 0; wEndX = x + margin;
   } else if(x > width-margin){
    wStartX = x - margin; wEndX = width-1;
   } else{
    wStartX = x - margin; wEndX = x + margin;
   }

   w = 0;
   for(int wy = wStartY; wy < wEndY; ++wy){
    for(int wx = wStartX; wx < wEndX; ++wx){

     window[w++] = rgbIn[wy][wx];
     printf("\n%d: w = %d\n", ID, w);

    }
   }

   printf("\n%d: Starting sort.\n", ID);
   std::sort(window, window + w);
   printf("\n%d: Finished sort.\n", ID);
   rgbOut[y][x] = window[w/2];
  }
 }*/

 // send to rank 0 (master):
 MPI_Send(rgbOut, height*width, MPI_BYTE, 0, TAG, MPI_COMM_WORLD);
 MPI_Recv(ack,1,MPI_BYTE,0,TAG,MPI_COMM_WORLD,&stat);
 printf("%d: Filtering complete, data sent back to master", ID);
}
//------------------------------------------------------------------------------

/** This is the entry point to the program. */
int main(int argc, char** argv){
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
 return 0;
}
//------------------------------------------------------------------------------
