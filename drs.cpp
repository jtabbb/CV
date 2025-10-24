/********************************************************************\

  Name:         drs_exam.cpp
  Created by:   Stefan Ritt

  Contents:     Simple example application to read out a DRS4
                evaluation board

  $Id: drs_exam.cpp 21308 2014-04-11 14:50:16Z ritt $

\********************************************************************/


#ifdef _MSC_VER

#include <windows.h>

#elif defined(OS_LINUX)

#define O_BINARY 0

#include <unistd.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DIR_SEPARATOR '/'

#endif

#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "strlcpy.h"
#include "DRS.h"
#include <math.h>
#include <cstdio>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <assert.h>
#include <iostream>
#include <cstdint>
#include <chrono>
#include <numeric>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>
#include <conio.h>  // For _kbhit() and _getch() functions (Windows only)
//#include "curses.h" //for linux

#pragma pack(push, 1)
struct RecordHeader {
    double timestamp;   // seconds since epoch
    int32_t channel;    // p index
    double trigger_level; //trigger level
    int32_t reserved;   // padding/alignment (optional)
};
#pragma pack(pop)
/*------------------------------------------------------------------*/

double mean(const std::vector<double>& data) {
    return std::accumulate(data.begin(), data.end(), 0.0) / data.size();
}

double stddev(const std::vector<double>& data, double mean_val) {
    double sum = 0.0;
    for (double val : data) {
        sum += (val - mean_val) * (val - mean_val);
    }
    return std::sqrt(sum / data.size());
}

double trapezoid_integral(const std::vector<double>& y, const std::vector<double>& x) {
    double integral = 0.0;
    for (size_t i = 1; i < y.size(); ++i) {
        integral += 0.5 * (y[i] + y[i - 1]) * (x[i] - x[i - 1]);
    }
    return integral;
}

int main()
{
    int i, l, n, p, numC, numCBB, nBoards, csel, f_ind, rub;
    double cbbtl;//cosmic background baseline trigger level
    char str[256];
    DRS* drs;
    DRSBoard* b;
    float time_array[8][1024];
    float wave_array[8][1024];
    FILE* f;
    FILE* fph;
    FILE* cbb;
    FILE* cbbWF;
    FILE* NM;

    /* do initial scan */
    drs = new DRS();

    /* show any found board(s) */
    for (i = 0; i < drs->GetNumberOfBoards(); i++) {
        b = drs->GetBoard(i);
        printf("Found DRS4 evaluation board, serial #%d, firmware revision %d\n",
            b->GetBoardSerialNumber(), b->GetFirmwareVersion());
    }

    /* exit if no board found */
    nBoards = drs->GetNumberOfBoards();
    if (nBoards == 0) {
        printf("No DRS4 evaluation board found\n");
        return 0;
    }

    /* continue working with first board only */
    b = drs->GetBoard(0);

    /* initialize board */
    b->Init();

    /* set sampling frequency */
    b->SetFrequency(0.7, true);

    /* enable transparent mode needed for analog trigger */
    b->SetTranspMode(1);

    /* set input range to -0.5V ... +0.5V */
    b->SetInputRange(0);
    //b->SetVoltageOffset(0,0);
    b->SetCalibVoltage(0.5);
    //b->EnableAcal(1, 0.5);
    /* use following line to turn on the internal 100 MHz clock connected to all channels  */
    b->EnableTcal(1);

   
    printf("To start data logging press Enter\n");
    std::getchar();
    // Main selection
    printf("Number of channels: ");
    fgets(str, sizeof(str), stdin);
    csel = atoi(str); //Number of channels
    printf("DRS4 configured for %d channels\n", atoi(str));

    b->EnableTrigger(1, 0);
    b->SetTriggerSource(768); // 15 is an OR on CH1-4, 3 is OR on CH1-2, apparently 768 is the AND value for CH1 and CH2 
    b->SetTriggerPolarity(true);        // positive edge -> false, negative edge -> true

    std::vector<int> avn = { 1, 2, 3, 4 };  // Available number of channels: 0, 2, 4, 6
    std::vector<double> triglist = { 0, 0, 0, 0 }; //Store trigger thresholds
    for (i = 0;i < csel;i++) {
        printf("Channel %d trigger level (V): ", avn[i]);
        fgets(str, sizeof(str), stdin);
        b->SetIndividualTriggerLevel((avn[i] - 1), atof(str));
        triglist[i] = atof(str) * -1000; //store as mV
        printf("Trigger level set to %1.3lf Volt\n", atof(str));
    }
    b->SetTriggerDelayNs(1200);             // zero ns trigger delay
    f_ind = 0;

    const int xoff2 = 50;
    const int valmean = 3;
    const int boff = 75;

    constexpr size_t N = 1024;
    std::vector<char> buffer;
   
    //std::ofstream ofs("output.bin", std::ios::binary);

    //std::ofstream ofs("output.bin", std::ios::binary | std::ios::app);
    std::ofstream ofs("output.bin", std::ios::binary | std::ios::trunc);
main_start:

    //File for saving Pk-Pk information and timestamps
    std::string filename = std::string("fph_") + std::to_string(f_ind) + ".txt";
    fph = fopen(filename.c_str(), "w");
    if (fph == NULL) {
        perror("ERROR: Cannot open file \"fph.txt\"");
        return 1;
    }
    //File for saving waveforms in main acquisition
    f = fopen("f.txt", "w");
    if (f == NULL) {
        perror("ERROR: Cannot open file \"f.txt\"");
        return 1;
    }

    numC = 0; //Event counter
    //std::vector<char> buffer;
    buffer.reserve(100 * csel * (sizeof(RecordHeader) + 2 * 1024 * sizeof(float)));
    for (i = 0;i < csel;i++) {
        fprintf(fph, "%7.1f,", triglist[i]);
    }
    std::vector<int> outtrig(csel, 0);
    for (int value : outtrig) {
        fprintf(fph, "%7d,", value);
    }

    fprintf(fph, "%d\n", 0);
    printf("Begin\n");
    //b->SetTriggerSource(1<<2);
    while (true) {
        //printf("In True\n");
        // Check if a key is pressed
        /*if (_kbhit()) {
            char ch = _getch();  // Read the pressed key without waiting for Enter
            if (ch == 'q')       // If 'q' is pressed, exit the loop
                break;
        }*/

        //initscr();
        //addstr("hit a key:");
        //getch();
        //return endwin();        

        //std::vector<double> outh(csel, 0); //Create output for pulse heights as 0s
        //std::vector<double> outpsd(csel, 0); //Create output for psd values as 0s
        //std::vector<double> outpkpk(csel, 0); //Create output for psd values as 0s
        //fflush(stdout);
        b->StartDomino();
        /* wait for trigger */
        while (b->IsBusy());
        /*Take time stamp*/
        auto now = std::chrono::system_clock::now();
        //auto start = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration<double>(now.time_since_epoch());
        double timeT = duration.count();
        //constexpr size_t N = 1024;

        /*Read waveforms*/
        b->TransferWaves(0, 8);
        b->GetTime(0, 0, b->GetTriggerCell(0), time_array[0]);
        //printf("Transfer waves\n");
        
        for (p = 0;p < csel;p++) {
            outtrig[p] = 0;
            b->GetWave(0, p * 2, wave_array[p]); // this reads channel i*2 to array index i
           // b->GetTime(0, p * 2, b->GetTriggerCell(0), time_array[p]);

            //std::ofstream ofs("output.bin", std::ios::binary | std::ios::app);

           // if (!ofs) {
           //     std::cerr << "Failed to open output file\n";
            //    continue;
            //}
            // Build header
            RecordHeader hdr;
            hdr.timestamp = timeT;
            hdr.channel = p;
            hdr.trigger_level = triglist[p];
            hdr.reserved = 0;
            //printf("%7.1f,", triglist[p]);
            // Write header + arrays
            //ofs.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
            //ofs.write(reinterpret_cast<const char*>(time_array[p]), sizeof(time_array[p]));
            //ofs.write(reinterpret_cast<const char*>(wave_array[p]), sizeof(wave_array[p]));
            //printf("After write\n");

            buffer.insert(buffer.end(),
                reinterpret_cast<const char*>(&hdr),
                reinterpret_cast<const char*>(&hdr) + sizeof(hdr));

            //buffer.insert(buffer.end(),
            //    reinterpret_cast<const char*>(time_array[p]),
            //    reinterpret_cast<const char*>(time_array[p]) + sizeof(time_array[p]));

            buffer.insert(buffer.end(),
                reinterpret_cast<const char*>(wave_array[p]),
                reinterpret_cast<const char*>(wave_array[p]) + sizeof(wave_array[p]));
            
            
        }
        buffer.insert(buffer.end(),
            reinterpret_cast<const char*>(time_array[0]),
            reinterpret_cast<const char*>(time_array[0]) + sizeof(time_array[0]));
        //auto end = std::chrono::high_resolution_clock::now();
        //std::chrono::duration<double> dur = end - start;
        //std::cout << "Duration: " << dur.count() << " seconds\n";
        //ofs.write(buffer.data(), buffer.size());

        //ofs.flush();
        //buffer.clear();
        //printf("N: %d\n", numC);

        numC++;
        if (numC >= 100) {
            ofs.write(buffer.data(), buffer.size());
            ofs.flush();         // optional for safety; can remove for more speed
            buffer.clear();      
            numC = 0;
            printf("Flushed 100 events to disk\n");
            
        }
        
    }
    fclose(fph);
    fclose(f);
    std::getchar();

    /* delete DRS object -> close USB connection */
    delete drs;
}
