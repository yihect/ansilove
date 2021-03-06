//
//  pcboard.c
//  AnsiLove/C
//
//  Copyright (C) 2011-2017 Stefan Vogt, Brian Cassidy, and Frederic Cambus.
//  All rights reserved.
//
//  This source code is licensed under the BSD 2-Clause License.
//  See the file LICENSE for details.
//

#include "pcboard.h"

void pcboard(unsigned char *inputFileBuffer, int32_t inputFileSize, char *outputFile, char *retinaout, char *font, int32_t bits, bool createRetinaRep)
{
    // some type declarations
    struct fontStruct fontData;
    int32_t columns = 80;
    int32_t loop, structIndex;

    // font selection
    alSelectFont(&fontData, font);

    // libgd image pointers
    gdImagePtr canvas;

    // process PCBoard
    int32_t character, current_character, next_character;
    int32_t background = 0, foreground = 7;
    int32_t column = 0, row = 0, columnMax = 0, rowMax = 0;

    // PCB buffer structure array definition
    struct pcbChar *pcboard_buffer, *temp;

    // PCB buffer dynamic memory allocation
    pcboard_buffer = malloc(sizeof(struct pcbChar));

    // reset loop
    loop = 0;
    structIndex = 0;

    while (loop < inputFileSize)
    {
        current_character = inputFileBuffer[loop];
        next_character = inputFileBuffer[loop+1];

        if (column == 80)
        {
            row++;
            column = 0;
        }

        // CR + LF
        if (current_character == 13 && next_character == 10) {
            row++;
            column = 0;
            loop++;
        }

        // LF
        if (current_character == 10)
        {
            row++;
            column = 0;
        }

        // Tab
        if (current_character==9)
        {
            column+=8;
        }

        // Sub
        if (current_character == 26)
        {
            break;
        }

        // PCB sequence
        if (current_character == 64 && next_character == 88)
        {
            // set graphics rendition
            background = inputFileBuffer[loop+2];
            foreground = inputFileBuffer[loop+3];
            loop+=3;
        }
        else if (current_character == 64 && next_character == 67 &&
                 inputFileBuffer[loop+2] == 'L' && inputFileBuffer[loop+3] == 'S')
        {
            // erase display
            column = 0;
            row = 0;

            columnMax = 0;
            rowMax = 0;

            loop+=4;
        }
        else if (current_character == 64 && next_character == 80 && inputFileBuffer[loop+2] == 'O'
                 && inputFileBuffer[loop+3] == 'S' && inputFileBuffer[loop+4]== ':')
        {
            // cursor position
            if (inputFileBuffer[loop+6]=='@')
            {
                column=((inputFileBuffer[loop+5])-48)-1;
                loop+=5;
            }
            else
            {
                column = (10 * ((inputFileBuffer[loop+5])-48) + (inputFileBuffer[loop+6])-48)-1;
                loop+=6;
            }
        }
        else if (current_character != 10 && current_character != 13 && current_character != 9)
        {
            // record number of columns and lines used
            if (column > columnMax)
            {
                columnMax = column;
            }

            if (row > rowMax)
            {
                rowMax = row;
            }

            // reallocate structure array memory
            temp = realloc(pcboard_buffer, (structIndex + 1) * sizeof(struct pcbChar));
            pcboard_buffer = temp;

            // write current character in pcbChar structure
            pcboard_buffer[structIndex].column = column;
            pcboard_buffer[structIndex].row = row;
            pcboard_buffer[structIndex].background = background;
            pcboard_buffer[structIndex].foreground = foreground;
            pcboard_buffer[structIndex].current_character = current_character;

            column++;
            structIndex++;
        }
        loop++;
    }
    columnMax++;
    rowMax++;

    // allocate buffer image memory
    canvas = gdImageCreate(columns * bits, (rowMax)*fontData.height);

    // allocate black color and create background canvas
    gdImageColorAllocate(canvas, 0, 0, 0);
    gdImageFill(canvas, 0, 0, 0);

    // allocate color palette
    int32_t colors[71];

    colors[48] = gdImageColorAllocate(canvas, 0, 0, 0);
    colors[49] = gdImageColorAllocate(canvas, 0, 0, 170);
    colors[50] = gdImageColorAllocate(canvas, 0, 170, 0);
    colors[51] = gdImageColorAllocate(canvas, 0, 170, 170);
    colors[52] = gdImageColorAllocate(canvas, 170, 0, 0);
    colors[53] = gdImageColorAllocate(canvas, 170, 0, 170);
    colors[54] = gdImageColorAllocate(canvas, 170, 85, 0);
    colors[55] = gdImageColorAllocate(canvas, 170, 170, 170);
    colors[56] = gdImageColorAllocate(canvas, 85, 85, 85);
    colors[57] = gdImageColorAllocate(canvas, 85, 85, 255);
    colors[65] = gdImageColorAllocate(canvas, 85, 255, 85);
    colors[66] = gdImageColorAllocate(canvas, 85, 255, 255);
    colors[67] = gdImageColorAllocate(canvas, 255, 85, 85);
    colors[68] = gdImageColorAllocate(canvas, 255, 85, 255);
    colors[69] = gdImageColorAllocate(canvas, 255, 255, 85);
    colors[70] = gdImageColorAllocate(canvas, 255, 255, 255);

    // the last value of loop tells us how many items are stored in there
    int32_t pcbBufferItems = structIndex;

    // render PCB
    for (loop = 0; loop < pcbBufferItems; loop++)
    {
        // grab our chars out of the structure
        column = pcboard_buffer[loop].column;
        row = pcboard_buffer[loop].row;
        background = pcboard_buffer[loop].background;
        foreground = pcboard_buffer[loop].foreground;
        character = pcboard_buffer[loop].current_character;

        drawchar(canvas, fontData.font_data, bits, fontData.height,
                   column, row, colors[background], colors[foreground], character);
    }

    // create output image
    output(canvas, outputFile, retinaout, createRetinaRep);

    // free memory
    free(pcboard_buffer);
}
