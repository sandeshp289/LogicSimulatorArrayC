///////////////////////////////////////////////////////////////////////////////////
// File       : logic_simulator_array.c
// Language   : C/C++
// Description: Event-Driven Combinational/Sequential Logic Simulator (Array-based)
// Author     : Sandesh Prabhakar
// Created    : November 8, 2007
// Modified   : August 15, 2026
// Version    : 1.0
//
// Copyright (c) 2026 Sandesh Prabhakar
// All rights reserved
////////////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum
{
   G_JUNK,         /* 0 */
   G_INPUT,        /* 1 */
   G_OUTPUT,       /* 2 */
   G_XOR,          /* 3 */
   G_XNOR,         /* 4 */
   G_DFF,          /* 5 */
   G_AND,          /* 6 */
   G_NAND,         /* 7 */
   G_OR,           /* 8 */
   G_NOR,          /* 9 */
   G_NOT,          /* 10 */
   G_BUF,          /* 11 */
   G_TIE1,         /* 12 */
   G_TIE0,         /* 13 */
};
void evaluate(int, unsigned char*, char*, char*, short*, int** );
////////////////////////////////////////////////////////////////////////
// circuit class
////////////////////////////////////////////////////////////////////////

class circuit
{
    // circuit information
    int numgates;       // total number of gates (faulty included)
    int maxlevels;      // number of levels in gate level ckt
    int numInputs;	// number of inputs
    int numOutputs;	// number of outputs
    int numDFFs; //number of DFFs
    unsigned char *gtype;	// gate type
    short *fanin;		// number of fanins of gate
    short *fanout;		// number of fanouts of gate
    int *levelNum;		// level number of gate
    int **faninlist;		// fanin list of gate
    int **fanoutlist;		// fanout list of gate

    char *value;        //value at the output of gate
    int EventList[1000][1000];   //event list for each level
    int gate_at_level[5000][5000];

    // you may wish to add more private variables here

public:
    circuit(char *);            // constructor
    // you may wish to add more function declarations (or public variables) here
};

////////////////////////////////////////////////////////////////////////
// constructor: reads in the *.lev file and builds basic data structure
// 		for the gate-level ckt
////////////////////////////////////////////////////////////////////////
circuit::circuit(char *cktName)
{
    FILE *inFile;
    char fName[40];
    int i, j, js, k, kk, nn, zz, h, r, count, lines;
    int d = 0;
    int p = 0;

    int df[5000];
    int pout[5000];
    int num_gate_per_level[5000];

    char c, temp, temp1, g1[3000];
    int gatenum, junk;
    int f1, f2, f3;

    strcpy(fName, cktName);
    strcat(fName, ".lev");
    inFile = fopen(fName, "r");
    if (inFile == NULL)
    {
	fprintf(stderr, "Can't open .lev file\n");
	exit(-1);
    }


    numgates = maxlevels = 0;

    fscanf(inFile, "%d", &count);	// number of gates

    fscanf(inFile, "%d", &junk);	// skip the second line


    // allocate space for gates data structure
    gtype = new unsigned char[count];
    fanin = new short[count];
    fanout = new short[count];
    levelNum = new int[count];
    faninlist = new int * [count];
    fanoutlist = new int * [count];
    value = new char[count];

    // now read in the circuit
    for (i=1; i<count; i++)
    {
	fscanf(inFile, "%d", &gatenum);
	fscanf(inFile, "%d", &f1);
	fscanf(inFile, "%d", &f2);
	fscanf(inFile, "%d", &f3);

	numgates++;

        //initializing the output of gates to X
        value[gatenum]='X';

	gtype[gatenum] = (unsigned char) f1;
	if (gtype[gatenum] > 13)
	    printf("gate %d is an unimplemented gate type\n", gatenum);
	else if (gtype[gatenum] == G_INPUT)
	    numInputs++;
	else if (gtype[gatenum] == G_OUTPUT)
	    {
	    numOutputs++;
	    pout[numOutputs] = gatenum;
        }
    else if (gtype[gatenum] == G_DFF)
        {
		numDFFs++;
		df[numDFFs] = gatenum;
        }

	f2 = (int) f2;
	levelNum[gatenum] = f2;

    //number of gates in each level
    num_gate_per_level[f2] = num_gate_per_level[f2] + 1;
	gate_at_level[f2][num_gate_per_level[f2]] = gatenum;

	if (f2 >= (maxlevels))
	    maxlevels = f2 + 5;

	fanin[gatenum] = (int) f3;
	// now read in the faninlist
	faninlist[gatenum] = new int[fanin[gatenum]];
	for (j=0; j<fanin[gatenum]; j++)
	{
	    fscanf(inFile, "%d", &f1);
	    faninlist[gatenum][j] = (int) f1;
	}

	for (j=0; j<fanin[gatenum]; j++) // followed by samethings
	    fscanf(inFile, "%d", &junk);

	// read in the fanout list
	fscanf(inFile, "%d", &f1);
	fanout[gatenum] = (int) f1;

	fanoutlist[gatenum] = new int[fanout[gatenum]];
	for (j=0; j<fanout[gatenum]; j++)
	{
	    fscanf(inFile, "%d", &f1);
	    fanoutlist[gatenum][j] = (int) f1;
	}

	// skip till end of line
	while ((c = getc(inFile)) != '\n' && c != EOF)
	    ;
    }	// for (i...)
    fclose(inFile);

    printf("Successfully read in circuit:\n");
    printf("\t%d PIs.\n", numInputs);
    printf("\t%d POs.\n", numOutputs);
    printf("\t%d DFFs \n", numDFFs);
    printf("\t%d total number of gates\n", numgates);
    printf("\t%d levels in the circuit.\n", maxlevels / 5);

    // (LOGIC SIMULATOR CODE STARTS HERE)

    int polevel = maxlevels - 5;

    strcpy(fName, cktName);
	strcat(fName, ".vec");
	inFile = fopen(fName, "r");
	if (inFile == NULL)
	  {
		fprintf(stderr, "Can't open .vec file\n");
		exit(-1);
	  }

        lines = 0;
        junk = 's'; 

	fscanf(inFile, "%d", &count);	// number of bits in each vector

        while (junk!='\n')
	fscanf(inFile, "%c", &junk);	// reading space and newline char


	while ((c = getc(inFile)) != 'E')
	  {
		//initializing the Event List
		    for (i=0; i<=(maxlevels-5); i=i+5)
		         {
		           for (js=1; js<=num_gate_per_level[i]; js++)
		             {
			       j = gate_at_level[i][js];
		               if (levelNum[j]==i && (i==0 || i==polevel))
		                  EventList[i][j]=j;
		   	       else
		   	          EventList[i][j]=0;
		   	     }
		         }

		         lines = lines + 1;
	      g1[1]=c;

	      //Reading the input vector

	      printf("\nInput Vector   %d   %c", lines-1, g1[1]);
	       for (i=2; i<=count; i++)
		     {
		     fscanf(inFile, "%c", &g1[i]);
		     printf("%c", g1[i]);
                     }

            junk = 's';
            while (junk!='\n')
	    fscanf(inFile, "%c", &junk);	// reading space and newline char

            for (i=0; i<=(maxlevels-5); i=i+5)
            { //for loop 'maxlevels-5' begins


		 for (js=1; js<=num_gate_per_level[i]; js++)
		     { //for loop 'numgates' begins

		      j=gate_at_level[i][js];

		       if (EventList[i][j]==j)
		         { //if loop begins

					temp=value[j];

                                        evaluate(j, gtype, value, g1, fanin, faninlist );

					 if (temp==value[j])
					     {
							 for (kk=0;kk<fanout[j]; kk++)
							    {
						     		h=fanoutlist[j][kk];
									r=levelNum[h];
									if ((EventList[r][h]==h))
									  break;
									  else
									EventList[r][h]=0;

						        }

						 }
					else
						  {

						  for (kk=0;kk<fanout[j]; kk++)
							    {
						     		h=fanoutlist[j][kk];
									r=levelNum[h];
									EventList[r][h]=h;
						        }

					      }

			      } //if loop ends

		      } //for loop 'numgates' ends

          } //for loop 'maxlevels-5' ends

           //Printing out the Primary output values corresponding to each vector

           printf("\nOutput Vector  %d   ",lines-1);
           for (zz=1; zz <=numOutputs; zz++)
	           printf("%c", value[pout[zz]]);

		   printf("\n");

           if(numDFFs>0)
            {
	   for (zz=1; zz <=numDFFs; zz++)
	   	   printf("%c", value[df[zz]]);
            }
	       printf("\n");

       } // while loop getc ends

       fclose(inFile);
       printf("\n");

       //If it is a sequential circuit containing D F/Fs, the following section section will also get executed
       if (numDFFs > 0)
         {
	   printf("Final Next State \n");
	   	      for (zz=1; zz <=numDFFs; zz++)
	   	      printf("%c", value[faninlist[df[zz]][0]]);
	 }
        printf("\n \nTotal number of patterns is %d\n", lines);

} //closing brace of constructor

////////////////////////////////////////////////////////////////////////
// other member functions here
////////////////////////////////////////////////////////////////////////
void evaluate (int j, unsigned char* gtype, char *value, char* g1, short* fanin, int** faninlist )
   {
    int nn;
    char out;
    bool out_int;

         switch (gtype[j])
              { //switch begins
			 //INPUT
		          case 1:
		          value[j]=g1[j];
		          break;

                          //NAND gate
			  case 7:
                          out='0';
			  for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (value[faninlist[j][nn]] == '0')
					 {
					 value[j]='1';
					 return;
                                         }
                                         else if (value[faninlist[j][nn]] == 'X')
                                         out='X';
				     }
                                         value[j]=out; 
					 break;

                              //AND gate
                              case 6:

                                     out='1';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (value[faninlist[j][nn]] == '0')
					 {
					 value[j]='0';
					 return;
                                         }
                                         else if (value[faninlist[j][nn]] == 'X')
                                         out='X';
				     }
                                         value[j]=out; 
					 break;

                              //OR gate
                              case 8:

                                     out='0';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (value[faninlist[j][nn]] == '1')
					 {
					 value[j]='1';
					 return;
                                         }
                                         else if (value[faninlist[j][nn]] == 'X')
                                         out='X';
				     }
                                         value[j]=out; 
					 break;

                              //NOR gate
                              case 9:
                                     out='1';
			             for (nn=0; nn<=(fanin[j]-1);nn++)
				     {
                                        if (value[faninlist[j][nn]] == '1')
					 {
					 value[j]='0';
					 return;
                                         }
                                         else if (value[faninlist[j][nn]] == 'X')
                                         out='X';
				     }
                                         value[j]=out; 
					 break; 

                              //XOR gate
			            case 3:
                                      out_int=0;
                                      for (nn=0; nn<=(fanin[j]-1);nn++)
				         {
                                        if (value[faninlist[j][nn]] == 'X')
					 {
					 value[j]='X';
					 return;
                                         }
                                         else if (value[faninlist[j][nn]] == '1')
                                         out_int=!out_int;
				        }

                                        switch (out_int)
                                            {
                                         case 0: out = '0';
                                                 break;
                                         case 1: out = '1';
                                                 break;
                                            }
                                         value[j]=out; 
					 break; 

                             //XNOR gate
				    case 4:
				      out_int=0;
                                      for (nn=0; nn<=(fanin[j]-1);nn++)
				         {
                                        if (value[faninlist[j][nn]] == 'X')
					 {
					 value[j]='X';
					 return;
                                         }
                                         else if (value[faninlist[j][nn]] == '1')
                                         out_int=!out_int;
				        }

                                        switch (out_int)
                                            {
                                         case 0: out = '1';
                                                 break;
                                         case 1: out = '0';
                                                 break;
                                            }
                                         value[j]=out; 
					 break; 

			      //NOT gate
                              case 10:
				        if ( value[faninlist[j][0]] == '0')
				  	       {
					        value[j]='1';
					       }
					else if (value[faninlist[j][0]] == '1')
					       value[j]='0';
					else if (value[faninlist[j][0]] == 'X')
						{
						value[j]='X';
					        }
					break;

                             //BUF gate
			      case 11:

				  	value[j]=value[faninlist[j][0]];
					break;

                              //TIE_1
                              case 12:
					value[j]='1';
					break;

                              //TIE_0
			      case 13:
				        value[j] = '0';
				        break;

                              //OUTPUT
			      case 2:
				        value[j]=value[faninlist[j][0]];
				        break;

			      //D_FF
			      case 5:
                                        value[j]=value[faninlist[j][0]];
				        break;

			   } //switch ends

		} //function ends

////////////////////////////////////////////////////////////////////////
// main starts here
////////////////////////////////////////////////////////////////////////
main(int argc, char *argv[])
{

    char cktName[20];
    circuit *ckt;

    int vecWidth, poNum;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <ckt>\n", argv[0]);
        exit(-1);
    }

    strcpy(cktName, argv[1]);

    // read in the circuit and build data structure
    ckt = new circuit(cktName);

    // NOW ADD YOUR FUNCTION HERE

}
