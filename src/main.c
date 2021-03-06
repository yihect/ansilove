//
//  main.c
//  AnsiLove/C
//
//  Copyright (C) 2011-2017 Stefan Vogt, Brian Cassidy, and Frederic Cambus.
//  All rights reserved.
//
//  This source code is licensed under the BSD 2-Clause License.
//  See the file LICENSE for details.
//

#define _XOPEN_SOURCE 700
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>
#include <unistd.h>
#include <err.h>

#ifndef HAVE_PLEDGE
#include "pledge.h"
#endif

#ifndef HAVE_STRTONUM
#include "strtonum.h"
#endif

#include "config.h"
#include "strtolower.h"
#include "ansilove.h"
#include "sauce.h"

#include "loaders/ansi.h"
#include "loaders/artworx.h"
#include "loaders/binary.h"
#include "loaders/icedraw.h"
#include "loaders/pcboard.h"
#include "loaders/tundra.h"
#include "loaders/xbin.h"

// prototypes
void showHelp(void);
void listExamples(void);
void versionInfo(void);
void synopsis(void);

void showHelp(void) {
    fprintf(stderr, "\nSUPPORTED FILE TYPES:\n"
           "  ANS   PCB   BIN   ADF   IDF   TND   XB\n"
           "  Files with custom suffix default to the ANSI renderer.\n\n"
           "PC FONTS:\n"
           "  80x25              icelandic\n"
           "  80x50              latin1\n"
           "  baltic             latin2\n"
           "  cyrillic           nordic\n"
           "  french-canadian    portuguese\n"
           "  greek              russian\n"
           "  greek-869          terminus\n"
           "  hebrew             turkish\n\n"
           "AMIGA FONTS:\n"
           "  amiga              topaz\n"
           "  microknight        topaz+\n"
           "  microknight+       topaz500\n"
           "  mosoul             topaz500+\n"
           "  pot-noodle\n\n"
           "DOCUMENTATION:\n"
           "  Detailed help is available at the AnsiLove/C repository on GitHub.\n"
           "  <https://github.com/ansilove/ansilove>\n\n");
}

void listExamples(void) {
    fprintf(stderr, "\nEXAMPLES:\n");
    fprintf(stderr, "  ansilove file.ans (output path/name identical to input, no options)\n"
           "  ansilove -i file.ans (enable iCE colors)\n"
           "  ansilove -r file.ans (adds Retina @2x output file)\n"
           "  ansilove -o dir/file.png file.ans (custom path/name for output)\n"
           "  ansilove -s file.bin (just display SAUCE record, don't generate output)\n"
           "  ansilove -m transparent file.ans (render with transparent background)\n"
           "  ansilove -f amiga file.txt (custom font)\n"
           "  ansilove -f 80x50 -b 9 -c 320 -i file.bin (font, bits, columns, icecolors)\n"
           "\n");
}

void versionInfo(void) {
    fprintf(stderr, "All rights reserved.\n"
           "\nFork me on GitHub: <https://github.com/ansilove/ansilove>\n"
           "Bug reports: <https://github.com/ansilove/ansilove/issues>\n\n"
           "This is free software, released under the 2-Clause BSD license.\n"
           "<https://github.com/ansilove/ansilove/blob/master/LICENSE>\n\n");
}

// following the IEEE Std 1003.1 for utility conventions
void synopsis(void) {
    fprintf(stderr, "\nSYNOPSIS:\n"
           "  ansilove [options] file\n"
           "  ansilove -e | -h | -v\n\n"
           "OPTIONS:\n"
           "  -b bits     set to 9 to render 9th column of block characters (default: 8)\n"
           "  -c columns  adjust number of columns for BIN files (default: 160)\n"
           "  -e          print a list of examples\n"
           "  -f font     select font (default: 80x25)\n"
           "  -h          show help\n"
           "  -i          enable iCE colors\n"
           "  -m mode     set rendering mode for ANS files:\n"
           "                ced            black on gray, with 78 columns\n"
           "                transparent    render with transparent background\n"
           "                workbench      use Amiga Workbench palette\n"
           "  -o file     specify output filename/path\n"
           "  -r          creates additional Retina @2x output file\n"
           "  -s          show SAUCE record without generating output\n"
           "  -v          show version information\n"
           "\n");
}

int main(int argc, char *argv[]) {
    fprintf(stderr, "AnsiLove/C %s - ANSI / ASCII art to PNG converter\n"\
           "Copyright (C) 2011-2017 Stefan Vogt, Brian Cassidy, and Frederic Cambus.\n", VERSION);

    // SAUCE record related bool types
    bool justDisplaySAUCE = false;
    bool fileHasSAUCE = false;

    // retina output bool type
    bool createRetinaRep = false;

    // iCE colors bool type
    bool icecolors = false;

    // analyze options and do what has to be done
    bool fileIsBinary = false;
    bool fileIsANSi = false;
    bool fileIsPCBoard = false;
    bool fileIsTundra = false;

    int getoptFlag;
    char *mode = NULL;
    char *font = NULL;

    char *input = NULL, *output = NULL;
    char *retinaout = NULL;

    char *outputFile;

    const char *errstr;

    // default to 8 if bits option is not specified
    int32_t bits = 8;

    // default to 160 if columns option is not specified
    int32_t columns = 160;

    if (pledge("stdio cpath rpath wpath", NULL) == -1) {
        err(EXIT_FAILURE, "pledge");
    }

    while ((getoptFlag = getopt(argc, argv, "b:c:ef:him:o:rsv")) != -1) {
        switch(getoptFlag) {
        case 'b':
            // convert numeric command line flags to integer values
            bits = strtonum(optarg, 8, 9, &errstr);

            if (errstr) {
                fprintf(stderr, "\nInvalid value for bits.\n\n");
                return EXIT_FAILURE;
            }

            break;
        case 'c':
            // convert numeric command line flags to integer values
            columns = strtonum(optarg, 1, 8192, &errstr);

            if (errstr) {
                fprintf(stderr, "\nInvalid value for columns.\n\n");
                return EXIT_FAILURE;
            }

            break;
        case 'e':
            listExamples();
            return EXIT_SUCCESS;
        case 'f':
            font = optarg;
            break;
        case 'h':
            showHelp();
            return EXIT_SUCCESS;
        case 'i':
            icecolors = true;
            break;
        case 'm':
            mode = optarg;
            break;
        case 'o':
            output = optarg;
            break;
        case 'r':
            createRetinaRep = true;
            break;
        case 's':
            justDisplaySAUCE = true;
            break;
        case 'v':
            versionInfo();
            return EXIT_SUCCESS;
        }
    }

    if (optind < argc) {
        input = argv[optind];
    } else {
        synopsis();
        return EXIT_SUCCESS;
    }

    argc -= optind;
    argv += optind;

    // let's check the file for a valid SAUCE record
    sauce *record = sauceReadFileName(input);

    // record == NULL also means there is no file, we can stop here
    if (record == NULL) {
        fprintf(stderr, "\nFile %s not found.\n\n", input);
        return EXIT_FAILURE;
    } else {
        // if we find a SAUCE record, update bool flag
        if (!strcmp(record->ID, SAUCE_ID)) {
            fileHasSAUCE = true;
        }
    }

    if (!justDisplaySAUCE) {
        // create output file name if output is not specified
        char *outputName;

        if (!output) {
            outputName = input;
            // appending ".png" extension to output file name
            int outputLen = strlen(outputName) + 5;
            outputFile = malloc(outputLen);
            snprintf(outputFile, outputLen, "%s%s", outputName, ".png");
        }
        else {
            outputName = output;
            outputFile = outputName;
        }

        if (createRetinaRep) {
            int retinaLen = strlen(outputName) + 8;
            retinaout = malloc(retinaLen);
            snprintf(retinaout, retinaLen, "%s%s", outputName, "@2x.png");
        }

        // default to empty string if mode option is not specified
        if (!mode) {
            mode = "";
        }

        // default to 80x25 font if font option is not specified
        if (!font) {
            font = "80x25";
        }

        // display name of input and output files
        fprintf(stderr, "\nInput File: %s\n", input);
        fprintf(stderr, "Output File: %s\n", outputFile);

        if (createRetinaRep) {
            fprintf(stderr, "Retina Output File: %s\n", retinaout);
        }

        // get file extension
        char *fext = strrchr(input, '.');
        fext = fext ? strtolower(strdup(fext)) : "";

        // load input file
        FILE *input_file = fopen(input, "r");
        if (input_file == NULL) {
            perror("File error");
            return 1;
        }

        // get the file size (bytes)
        struct stat input_file_stat;

        if (fstat(fileno(input_file), &input_file_stat)) {
            perror("Can't stat file");
            return 1;
        }
        size_t inputFileSize=input_file_stat.st_size;

        // next up is loading our file into a dynamically allocated memory buffer
        unsigned char *inputFileBuffer;

        // allocate memory to contain the whole file
        inputFileBuffer = (unsigned char *) malloc(sizeof(unsigned char)*inputFileSize + 1);
        if (inputFileBuffer == NULL) {
            perror("Memory error");
            return 2;
        }

        // copy the file into the buffer
        if (fread(inputFileBuffer, 1, inputFileSize, input_file) != inputFileSize) {
            perror("Reading error");
            return 3;
        } // whole file is now loaded into inputFileBuffer

        inputFileBuffer[inputFileSize] = '\0';

        // adjust the file size if file contains a SAUCE record
        if(fileHasSAUCE) {
            sauce *saucerec = sauceReadFile(input_file);
            inputFileSize -= 129 - ( saucerec->comments > 0 ? 5 + 64 * saucerec->comments : 0);
        }

        // close input file, we don't need it anymore
        fclose(input_file);

        // create the output file by invoking the appropiate function
        if (!strcmp(fext, ".pcb")) {
            // params: input, output, font, bits, icecolors
            pcboard(inputFileBuffer, inputFileSize, outputFile, retinaout, font, bits, createRetinaRep);
            fileIsPCBoard = true;
        } else if (!strcmp(fext, ".bin")) {
            // params: input, output, columns, font, bits, icecolors
            binary(inputFileBuffer, inputFileSize, outputFile, retinaout, columns, font, bits, icecolors, createRetinaRep);
            fileIsBinary = true;
        } else if (!strcmp(fext, ".adf")) {
            // params: input, output, bits
            artworx(inputFileBuffer, inputFileSize, outputFile, retinaout, createRetinaRep);
        } else if (!strcmp(fext, ".idf")) {
            // params: input, output, bits
            icedraw(inputFileBuffer, inputFileSize, outputFile, retinaout, createRetinaRep);
        } else if (!strcmp(fext, ".tnd")) {
            tundra(inputFileBuffer, inputFileSize, outputFile, retinaout, font, bits, createRetinaRep);
            fileIsTundra = true;
        } else if (!strcmp(fext, ".xb")) {
            // params: input, output, bits
            xbin(inputFileBuffer, inputFileSize, outputFile, retinaout, createRetinaRep);
        } else {
            // params: input, output, font, bits, icecolors, fext
            ansi(inputFileBuffer, inputFileSize, outputFile, retinaout, font, bits, mode, icecolors, fext, createRetinaRep);
            fileIsANSi = true;
        }

        // gather information and report to the command line
        if (fileIsANSi || fileIsBinary ||
            fileIsPCBoard || fileIsTundra) {
            fprintf(stderr, "Font: %s\n", font);
            fprintf(stderr, "Bits: %d\n", bits);
        }
        if (icecolors && (fileIsANSi || fileIsBinary)) {
            fprintf(stderr, "iCE Colors: enabled\n");
        }
        if (fileIsBinary) {
            fprintf(stderr, "Columns: %d\n", columns);
        }
    }

    // either display SAUCE or tell us if there is no record
    if (!fileHasSAUCE) {
        fprintf(stderr, "\nFile %s does not have a SAUCE record.\n", input);
    } else {
        fprintf(stderr, "\nId: %s v%s\n", record->ID, record->version);
        fprintf(stderr, "Title: %s\n", record->title );
        fprintf(stderr, "Author: %s\n", record->author);
        fprintf(stderr, "Group: %s\n", record->group);
        fprintf(stderr, "Date: %s\n", record->date);
        fprintf(stderr, "Datatype: %d\n", record->dataType);
        fprintf(stderr, "Filetype: %d\n", record->fileType);
        if (record->flags != 0) {
            fprintf(stderr, "Flags: %d\n", record->flags);
        }
        if (record->tinfo1 != 0) {
            fprintf(stderr, "Tinfo1: %d\n", record->tinfo1);
        }
        if (record->tinfo2 != 0) {
            fprintf(stderr, "Tinfo2: %d\n", record->tinfo2);
        }
        if (record->tinfo3 != 0) {
            fprintf(stderr, "Tinfo3: %d\n", record->tinfo3);
        }
        if (record->tinfo4 != 0) {
            fprintf(stderr, "Tinfo4: %d\n", record->tinfo4);
        }
        if (record->comments > 0) {
            fprintf(stderr, "Comments: ");
            for (int32_t i = 0; i < record->comments; i++) {
                fprintf(stderr, "%s\n", record->comment_lines[i] );
            }
        }
    }

    return EXIT_SUCCESS;
}
