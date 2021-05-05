/**
* This file is part of ORB-SLAM2.
*
* Copyright (C) 2014-2016 Raúl Mur-Artal <raulmur at unizar dot es> (University of Zaragoza)
* For more information see <https://github.com/raulmur/ORB_SLAM2>
*
* ORB-SLAM2 is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ORB-SLAM2 is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with ORB-SLAM2. If not, see <http://www.gnu.org/licenses/>.
*/


#include<iostream>
#include<algorithm>
#include<fstream>
#include<chrono>
#include<iomanip>

#include<opencv2/core/core.hpp>
#ifdef OPENCV4
#include <opencv2/imgcodecs/legacy/constants_c.h>
#endif

#include"System.h"

using namespace std;

void LoadImages(const string &strSequence, vector<string> &vstrImageFilenames,
                vector<double> &vTimestamps, double multiplier);
                
typedef struct {
	int    frame_offset = 0;
	double wait_time = 0;
	
	//state
	double time = 0;
	
	//constant
	int frames_to_advance = 1;
	bool realtime = true;
	bool skip = true;
} simulation_time_status;

typedef struct {
    simulation_time_status s;
    double fps_multiplier    = 1;
    double init_fps_slowdown = 1;
    bool include_img_load = true;
    std::string voc_file = "";
    std::string settings_file = "";
    std::string sequence_path = "";
    std::string times_file = "";
    
    int start_frame = 0;
    int end_frame = 0;
} mono_params;
  
/*
Legend:
  - |   Frame to be processed
  - -   Time between events
  - :   Frame to be skipped (to lower the framerate)
  - s   starting processing
  - e   ending processing
  - w   wait
  - *   continue elaboration
  - +   Time of frame to be processed but elaboration ongoing
  
Example (output FPS halved)
 > |----:----|----:----|----:----|----:----|

Case 1
  > s---ewwwwws********
   State:
  	 delay 0
   Out:
   	frames_offset 0 (able to process all)
    wait time     5
    delay         0
  	
Case 2
  > s---------+--es*****
   State:
  	 delay 0
   Out:
  	 frames_offset 0 (able to process all)
  	 wait time     0
  	 delay         3 (frame processing started 3 unit later)

Case 3
  > s---------+---------+-es******
   State:
  	 delay 0
   Out:
  	 frames_offset 1 (Missed completely a frame)
  	 wait time     0
  	 delay         2 (frame processing started 2 unit later)
*/

void simulation_step(double time_processed, std::vector<double> vTimestamps, int start_idx, simulation_time_status& s)
{
	s.frame_offset = 0;
	s.wait_time    = 0;
	
	int n_images = vTimestamps.size();
	int next_idx = start_idx + s.frames_to_advance;
	
	double start_time = s.time;//vTimestamps[start_idx];
	double final_time_processed = start_time + time_processed;
	
	if(next_idx >= n_images)
	{
		// End of the stream: special handling
		s.frame_offset = 0;
		s.wait_time = (s.skip) ? std::max(0.0, vTimestamps[n_images-1] - final_time_processed) : 0.0;//wait the complete acquisition
		s.time += time_processed + s.wait_time;
		return;
	}
	
	if(!s.skip)
	{
		//no skip frame, only wait if processing time was short enough
		s.wait_time = std::max(0.0, vTimestamps[next_idx] - ( vTimestamps[start_idx] + time_processed )); 
		//std::cout << "Wait time: " << s.wait_time << " " << vTimestamps[next_idx] << " " << vTimestamps[start_idx] << " " << time_processed << std::endl;
		s.time += time_processed + s.wait_time;
		return;
	}
	
	
	double next_time = vTimestamps[next_idx];
	if(final_time_processed <= next_time)
	{
		//case 1
		//processing did well, need to wait (THE ONLY CASE!) and no more delay!
		s.wait_time = next_time - final_time_processed;
		s.time += time_processed + s.wait_time;
	}
	else
	{
	    s.time += time_processed;
		//offset because do/while
		s.frame_offset = 0;
        double prev_time = vTimestamps[start_idx];
        double cur_time  = next_time;
        double delta_time = cur_time - prev_time;
		do
		{
			s.frame_offset += s.frames_to_advance;
			
			if(start_idx+1+s.frame_offset < n_images)
			    cur_time = vTimestamps[start_idx+1+s.frame_offset];
			else
			    cur_time = prev_time + delta_time;
			delta_time = cur_time - prev_time;
			prev_time  = cur_time;
		}while(cur_time < s.time);
		//recover a step if the time is not a perfect multiple
		//(if a perfect multiple, the execution finished when new frame is ready)
		if(cur_time != s.time) s.frame_offset -= s.frames_to_advance;
	}
}
mono_params ParseArgs(int argc, char **argv)
{
    mono_params p;
    p.s.frames_to_advance = 1;
    p.s.time = 0;
    p.s.realtime = true;
	p.s.skip     = false;
	p.include_img_load = true;
    p.fps_multiplier = 1;
    
    p.settings_file = argv[2];
    p.sequence_path = argv[3];
    
    if(argc >= 5)
    {
        p.fps_multiplier = atof(argv[4]);
    }
    if(argc >= 6)
    {
        p.s.frames_to_advance = atoi(argv[5])+1;
    }
    if(argc >= 7)
    {
        if(strcmp(argv[6], "skip") == 0)
        {
            p.s.skip = true;
        }
        else if(strcmp(argv[6], "skip_noimgtime") == 0)
        {
            p.s.skip = true;
            p.include_img_load = false;
        }
    }
    if(argc >= 8) {
        if(strcmp(argv[7], "no") == 0)
        {
            p.s.realtime = false;
        }
    }
    
    if(argc >= 9) {
        p.init_fps_slowdown = atof(argv[8]);
    }
    if(argc >= 10) {
		p.start_frame = atoi(argv[9]);
	} else {
		p.start_frame = 0;
	}
	
	if(argc >= 11) {
		p.end_frame = atoi(argv[10]);
	} else {
		p.end_frame = std::numeric_limits<int>::max();
	}
	
    
    std::cout << "Parameters: " << std::endl;
    std::cout << "- Setting file:      " << p.settings_file << std::endl;
    std::cout << "- Sequence path:     " << p.sequence_path << std::endl;
    std::cout << "- FPS multiplier:    " << p.fps_multiplier << std::endl;
    std::cout << "- Frames to skip:    " << p.s.frames_to_advance << std::endl;
    std::cout << "- Skip:              " << p.s.skip << std::endl;
    std::cout << "- Include img time:  " << p.include_img_load << std::endl;
    std::cout << "- Realtime:          " << p.s.realtime << std::endl;
    std::cout << "- FPS init slowdown: " << p.init_fps_slowdown << std::endl;
    
    return p;
}

int main(int argc, char **argv)
{
    if(argc /*!=*/ < 4)
    {
        cerr << endl << "Usage: ./mono_kitti path_to_vocabulary path_to_settings path_to_sequence [FPS multiplier: 1] [Frames to skip: 0] [Processing time: \"\"|skip|skip_noimgtime] [Realtime: \"\"|no] [start frame: 0] [end frame: n]" << endl;
        cerr << "Actual: " << std::endl;
        for(int i = 0; i < argc; ++i) {
			cerr << "- " << argv[i] << std::endl;
		}
        return 1;
    }
    mono_params p = ParseArgs(argc, argv);
    /*double multiplier = 1;
    if( argc > 4) {
		multiplier = atof(argv[4]);
		std::cout << "FPS multiplier" << multiplier << std::endl;
	}*/

    // Retrieve paths to images
    vector<string> vstrImageFilenames;
    vector<double> vTimestamps;
    LoadImages(string(argv[3]), vstrImageFilenames, vTimestamps, p.fps_multiplier);

    int nImages = vstrImageFilenames.size();
    
    
    p.end_frame = std::min(p.end_frame, nImages);
    nImages = p.end_frame;
    p.start_frame = std::min(p.start_frame, p.end_frame);
    p.s.time = vTimestamps[p.start_frame];
    
    // Create SLAM system. It initializes all system threads and gets ready to process frames.
    ORB_SLAM2::System SLAM(argv[1],argv[2],ORB_SLAM2::System::MONOCULAR,true);

    // CustomVector for tracking time statistics
    vector<float> vTimesTrack;
    vTimesTrack.resize(nImages);

    cout << endl << "-------" << endl;
    cout << "Start processing sequence ..." << endl;
    cout << "Images in the sequence: " << nImages << endl << endl;
    char c;
    //std::cin >> c;

    // Main loop
    cv::Mat im;
    for(int ni=p.start_frame; ni<nImages; ni++)
    {
		//cout << "Loading image " << ni << std::endl;
        // Read image from file
        auto start = std::chrono::steady_clock::now();
        im = cv::imread(vstrImageFilenames[ni],CV_LOAD_IMAGE_UNCHANGED);
        if(!p.include_img_load)
			start = std::chrono::steady_clock::now();
        double tframe = vTimestamps[ni];

        if(im.empty())
        {
            cerr << endl << "Failed to load image at: " << vstrImageFilenames[ni] << endl;
            return 1;
        }

#ifdef COMPILEDWITHC11
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();
#else
        std::chrono::monotonic_clock::time_point t1 = std::chrono::monotonic_clock::now();
#endif

        // Pass the image to the SLAM system
        SLAM.TrackMonocular(im,tframe, ni);

#ifdef COMPILEDWITHC11
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
#else
        std::chrono::monotonic_clock::time_point t2 = std::chrono::monotonic_clock::now();
#endif
		auto end = t2;

        double ttrack= std::chrono::duration_cast<std::chrono::duration<double> >(t2 - t1).count();

        vTimesTrack[ni]=ttrack;

		auto time_processed = std::chrono::duration<double, std::ratio<1>> (end - start).count();
		//auto tt1 = time_processed;
		//std::cout << time_processed << std::endl;
        // Wait to load the next frame
        if(ni < 20) {
			//time_processed = -1; //at initialization, give PLENTY of times; (1s per frame)
			//time_processed = 0; //for somewhat reason, this works...
			
			time_processed = //std::max(
				//std::chrono::milliseconds::zero().count(), 
				(
				   std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
				 - std::chrono::milliseconds(static_cast<int>((p.init_fps_slowdown-1)*1000*(vTimestamps[ni+1]-vTimestamps[ni])))
				).count()
				/1000.0;
				//*/
				//)/1000.0;//init to 1 FPS
				//std::cout << time_processed << std::endl;
			//double t = vTimestamps[ni - 
			//usleep(p.s.wait_time*1e6);
			
		}
		if(p.s.realtime)
		{
			simulation_step(time_processed, vTimestamps, ni, p.s);
			ni += p.s.frame_offset;
			if(p.s.frame_offset > 0)
			{
				//std::cout << "Skipping " << s.frame_offset << " frames" << std::endl;
			}
			//if(ni < 20) {
			//	std::cout << "FPS INIT: " << 1/(tt1 + p.s.wait_time) << std::endl;
			//}
			/*std::cout << "TIME:       " << tt1 + p.s.wait_time << std::endl;
			std::cout << "Wait time:  " << p.s.wait_time << " s" << std::endl;
			std::cout << "Time clock: " << p.s.time << " s" << std::endl;
			if(ni < 1100)
			std::cout << "Time frame: " << vTimestamps[ni+1] << " s" << std::endl;*/
			if(p.s.wait_time > 0)
				usleep(p.s.wait_time*1e6);
		}
            
        //cout << "End image " << ni << std::endl;
    }

    // Stop all threads
    SLAM.Shutdown();
    
    {
		ofstream f;
		f.open("times.txt");
		f << "nframe" << " " << "init" << " " << "lost" << " " << "timestamp" << " " << "kf" << " " << "kf_stopped" << " " << "elaborated" << endl;
		f << fixed;

		list<int>::iterator lis = SLAM.mpTracker->mliSequenceID.begin();
		list<ORB_SLAM2::KeyFrame*>::iterator lkf = SLAM.mpTracker->mlpReferences.begin();
		list<int>::iterator lisMapB = SLAM.mpTracker->mliSequenceIDMap.begin();
		bool init = false;
		bool lost = false;
		bool kf = false;
		bool kf_requested = false;
		
		int nframe = 1;
		for(auto lit = SLAM.mpTracker->mlbLost.begin(),  lend = SLAM.mpTracker->mlbLost.end();lit!=lend;lit++, lis++, lkf++)
		{
			int seqId = (*lis)+1;
			kf = false;
			kf_requested = false;
			
			while(nframe < seqId) {
				f << nframe << " " << init << " " << lost << " 0 " << vTimesTrack[nframe-1] << " " << kf << " " << kf_requested << " 0" << endl;
				nframe++;
			}
			init = true;
			lost = *lit;
			
			kf = (*lkf)->sequenceID == (nframe-1);
			kf_requested = std::any_of(SLAM.mpTracker->mliSequenceIDMap.begin(), SLAM.mpTracker->mliSequenceIDMap.end(), [&](int i) {
				return i == (nframe-1);
			});
			
			f << nframe << " " << init << " " << lost << " 1 " << vTimesTrack[nframe-1] << " " << kf << " " << kf_requested << " 1" << endl;
			nframe++;
		}
		{
			while(nframe <= nImages) { 
				f << nframe << " " << init << " " << lost << " 0 " << vTimesTrack[nframe-1] << " " << kf << " " << kf_requested << " 0" << endl;
				nframe++;
			}
		}
		f.close();
	}

    // Tracking time statistics
    sort(vTimesTrack.begin(),vTimesTrack.end());
    float totaltime = 0;
    for(int ni=0; ni<nImages; ni++)
    {
        totaltime+=vTimesTrack[ni];
    }
    cout << "-------" << endl << endl;
    cout << "median tracking time: " << vTimesTrack[nImages/2] << endl;
    cout << "mean tracking time: " << totaltime/nImages << endl;

    // Save camera trajectory
    SLAM.SaveKeyFrameTrajectoryTUM("kf_trajectory.tum", p.fps_multiplier);    
    SLAM.SaveTrajectoryTUM("trajectory.tum", p.fps_multiplier);
    SLAM.SaveTrajectoryTUMLost("trajectory_lost.tum", p.fps_multiplier);
    //SLAM.SaveKeyFrameTrajectoryKITTI("kf_trajectory.kitti");    
    SLAM.SaveTrajectoryKITTI("trajectory.kitti");

    return 0;
}

void LoadImages(const string &strPathToSequence, vector<string> &vstrImageFilenames, vector<double> &vTimestamps, double multiplier)
{
    ifstream fTimes;
    string strPathTimeFile = strPathToSequence + "/times.txt";
    fTimes.open(strPathTimeFile.c_str());
    while(!fTimes.eof())
    {
        string s;
        getline(fTimes,s);
        if(!s.empty())
        {
            stringstream ss;
            ss << s;
            double t;
            ss >> t;
            t *= multiplier;
            vTimestamps.push_back(t);
        }
    }

    string strPrefixLeft = strPathToSequence + "/image_0/";

    const int nTimes = vTimestamps.size();
    vstrImageFilenames.resize(nTimes);

    for(int i=0; i<nTimes; i++)
    {
        stringstream ss;
        ss << setfill('0') << setw(6) << i;
        vstrImageFilenames[i] = strPrefixLeft + ss.str() + ".png";
    }
}
