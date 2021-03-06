/*
 * Software License Agreement (BSD License)
 * 
 * Copyright (c)  2012, Wongun Choi
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies, 
 * either expressed or implied, of the FreeBSD Project.
 */

#include <tracker/target_manager.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <common/assignmentoptimal.h>
#include <common/util.h>
#include <boost/lexical_cast.hpp>

using namespace people;

#define INF_DIST	100000000.0

Target::Target(int tid):id_(tid),status_(TargetTracking)
{
	//patch_ = cv::Mat(100, 100, CV_8UC3);
	//patch_ = cv::Scalar(255, 255, 255);
	init_ = false;
}

Target::Target(const Target &target)
{
	id_ = target.id_;
	status_ = target.status_;
	states_ = target.states_;
	rts_ = target.rts_;
	//patch_ = target.patch_;
	init_ = target.init_;
}

Target::~Target()
{
}

void Target::setLocation(ObjectStatePtr loc, cv::Rect rt, cv::Mat image)
{
	states_.push_back(loc);
	rts_.push_back(rt);

	cv::MatND hist_out;
	if(buildHistogram(image, rt, init_, hist_, hist_out, 0.5)) {
		hist_ = hist_out;
		init_ = true;
	}
#if 0 
	if(within_image(image, rt)) {
		cv::Mat patch(image, rt);
		cv::resize(patch, patch_, cv::Size(100, 100));
		init_ = true;
	}
	else if(init_ == false) {
		cv::Rect rt_temp(0, 0, image.cols, image.rows);
		rt_temp = rt_temp & rt;

		cv::Mat patch(image, rt_temp);
		cv::resize(patch, patch_, cv::Size(100, 100));
	}
#endif
}

bool Target::writeToFile(std::string filename)
{
	std::ofstream out(filename.c_str()); 
	if(!out) { 
		cout << "Target::writeToFile - Cannot open file " << filename << std::endl; 
		return false;
	} 
	for(size_t i = 0; i < states_.size(); i++)
	{
		out << std::setprecision(30) 
				  << states_[i]->getTS() << " " ;
		// write states
		for(size_t j = 0; j < states_[i]->numElement(); j++)
			out << states_[i]->getElement(j) << " ";
		// write rect and confidence
		out << rts_[i].x << " "
			  << rts_[i].y << " "
			  << rts_[i].width << " "
			  << rts_[i].height << " "
			  << states_[i]->getConfidence() << " "
			  << std::endl;
	}
	out.close();

	return true;
}

double Target::computeColorSim(const cv::Mat &image, const cv::Rect &rt)
{
	double sim = 0.0;
	if(!init_) return 1.0; // no information yet...

	cv::MatND hist_rt;
	if(buildHistogram(image, rt, false, cv::MatND(), hist_rt, 0.0)) {
		sim = computeSim(hist_rt, hist_);
	}

	return sim;
}

double Target::getOverlap(const cv::Rect &rt)
{
	cv::Rect last_rt = rts_[rts_.size() - 1];
	cv::Rect det = rt;

	return bb_overlap(det, last_rt);
}

double Target::getDistance(ObjectStatePtr state)
{
	ObjectStatePtr last_state = states_[states_.size() - 1];
	return state_dist(last_state, state);
}

int Target::findIdx(double ts)
{
	for(size_t idx = 0; idx < states_.size(); idx++) {
		if(fabs(states_[idx]->getTS() - ts) < 0.01)
			return idx;
	}
	return -1;
}

///////////////////////////
Feature::Feature(int tid):id_(tid)
{
}

Feature::Feature(const Feature &feat)
{
	id_ = feat.id_;
	states_ = feat.states_;
	projs_ = feat.projs_;
	obs_ = feat.obs_;
}

Feature::~Feature()
{
}

void Feature::setState(FeatureStatePtr state, cv::Point2f proj, cv::Point2f obs)
{
	states_.push_back(state);
	projs_.push_back(proj);
	obs_.push_back(obs);
}

bool Feature::writeToFile(std::string filename)
{
	std::ofstream out(filename.c_str()); 

	if(!out) { 
		cout << "Feature::writeToFile - Cannot open file " << filename << std::endl; 
		return false;
	} 
	
	for(size_t i = 0; i < states_.size(); i++)
	{
		out << std::setprecision(30) 
				  << states_[i]->getTS() << " ";
		// write states
		for(size_t j = 0; j < states_[i]->numElement(); j++)
			out << states_[i]->getElement(j) << " ";
		// write confidence and position
		out << states_[i]->getConfidence() << " "
			  << projs_[i].x << " "
			  << projs_[i].y << " "
			  << obs_[i].x << " "
			  << obs_[i].y << " "
			  << std::endl;
	}
	out.close();

	return true;
}

int Feature::findIdx(double ts)
{
	for(size_t idx = 0; idx < states_.size(); idx++) {
		if(fabs(states_[idx]->getTS() - ts) < 0.01)
			return idx;
	}
	return -1;
}
////////////////////////////////
Camera::Camera()
{
}

Camera::~Camera()
{
}

void Camera::setState(CameraStatePtr cam)
{
	states_.push_back(cam);
}

bool Camera::writeToFile(std::string filename)
{
	std::ofstream out(filename.c_str()); 

	if(!out) { 
		cout << "Camera::writeToFile - Cannot open file " << filename << std::endl; 
		return false;
	} 
	
	for(size_t i = 0; i < states_.size(); i++)
	{
		out << std::setprecision(30) 
			  << states_[i]->getTS() << " ";
		// write states
		for(size_t j = 0; j < states_[i]->numElement(); j++)
			out << states_[i]->getElement(j) << " ";
		out << std::endl;
	}
	out.close();

	return true;

}

int Camera::findIdx(double ts)
{
	for(size_t idx = 0; idx < states_.size(); idx++) {
		if(fabs(states_[idx]->getTS() - ts) < 0.01)
			return idx;
	}
	return -1;
}

Lines::Lines()
{
}

Lines::Lines(double timesec)
{
	timesec_ = timesec;
}

Lines::Lines(const Lines &src)
{
	timesec_ = src.timesec_;
	lines_ = src.lines_;
	lanes_ = src.lanes_;
}

Lines::~Lines()
{
}

void Lines::setTimesec(double timesec)
{
	timesec_ = timesec;
}

double Lines::getTimesec()
{
	return timesec_;
}

void Lines::setLines(const std::vector<longline> &lines)
{
	lines_ = lines;
}

void Lines::setLanes(const std::vector<longline> &lanes)
{
	lanes_ = lanes;
}

// bool Lines::writeLanesToFile(std::string filename)
bool Lines::writeLanesToFile(std::ofstream &out)
{
#if 0
	std::ofstream out(filename.c_str()); 
	if(!out) { 
		cout << "Line::writeLanesToFile - Cannot open file " << filename << std::endl; 
		return false;
	} 
#endif
	out << std::setprecision(30) 
		<< timesec_ << " " 
		<< lanes_.size() << " "
		<< std::endl;

	for(size_t i = 0; i < lanes_.size(); i++) {
		out << std::setprecision(30) 
				  << lanes_[i].x1_ << " "
				  << lanes_[i].x2_ << " "
				  << lanes_[i].y1_ << " "
				  << lanes_[i].y2_ << " "
				  << lanes_[i].angle_ << " "
				  << lanes_[i].length_ << " "
				  << std::endl;
	}
	return true;
}

// bool Lines::writeLinesToFile(std::string filename)
bool Lines::writeLinesToFile(std::ofstream &out)
{
#if 0
	std::ofstream out(filename.c_str()); 
	if(!out) { 
		cout << "Line::writeLinesToFile - Cannot open file " << filename << std::endl; 
		return false;
	} 
#endif
	out << std::setprecision(30) 
		<< timesec_ << " " 
		<< lines_.size() << " "
		<< std::endl;

	for(size_t i = 0; i < lines_.size(); i++) {
		out << std::setprecision(30) 
				  << lines_[i].x1_ << " "
				  << lines_[i].x2_ << " "
				  << lines_[i].y1_ << " "
				  << lines_[i].y2_ << " "
				  << lines_[i].angle_ << " "
				  << lines_[i].length_ << " "
				  << std::endl;
	}

	return true;
}

////////////////////////////////
TargetManager::TargetManager()
{
	max_dist_ = 0.3;
	min_overlap_ = 0.4;
}

TargetManager::TargetManager(const TargetManager &src)
{
	targets_ = src.targets_;
}

TargetManager::~TargetManager()
{
}

void TargetManager::updateCamera(CameraStatePtr cam_hat)
{
	cam_.setState(cam_hat);
	// gt_cam_.setState(cam_gt);
}

void TargetManager::updateFeatures(const std::vector<int> &feat_idx, const std::vector<FeatureDistPtr> &featDists, const std::vector<cv::Point2f> &proj, const std::vector<cv::Point2f> &obs)
{
	for(size_t i = 0; i < featDists.size(); i++) {
		int idx = -1;
		for(size_t j = 0; j < feats_.size(); j++) {
			if(feats_[j]->getID() == feat_idx[i]) {
				idx = j;
				break;
			}
		}
		if(idx < 0) {
			// new feature
			FeaturePtr new_feat = boost::make_shared<Feature>(feat_idx[i]);
			new_feat->setState(featDists[i]->getMean(), proj[i], obs[i]);
			feats_.push_back(new_feat);
		}
		else {
			// existing feature
			feats_[idx]->setState(featDists[i]->getMean(), proj[i], obs[i]);
		}
	}
}

void TargetManager::updateTargets(const std::vector<TargetDistPtr> &targetDists, const std::vector<cv::Rect> &rts, const cv::Mat &image)
{
	size_t cnt = 0;
#ifdef _DEBUG
	std::cout << "updateTargets:new target cnt " << targetDists.size() << " ext target cnt " << targets_.size() << std::endl;
#endif
	for(size_t i = 0; i < targets_.size(); i++) {
		if(targets_[i]->getStatus() == TargetTracking) {
			assert(cnt < targetDists.size());
			// targetDists[cnt] --> targets_[i]
			targets_[i]->setLocation(targetDists[cnt]->getMean(), rts[cnt], image);
			cnt++;
		}
	}
	// add new targets.
	for(; cnt < targetDists.size(); cnt++) {
		TargetPtr new_target = boost::make_shared<Target>(Target(targets_.size()));
		new_target->setLocation(targetDists[cnt]->getMean(), rts[cnt], image);
		targets_.push_back(new_target);
	}
}

void TargetManager::updateMeanShiftTrackers(cv::Mat &image_color)
{
	for(size_t i = 0; i < targets_.size(); i++) {
		if(i < ms_targets_.size()) {
			// existing targets
		}
		else {
			// newly entered target
			ms_targets_.push_back(MSTarget());
			my_assert(targets_[i]->getNumStates() == 1);
		}
		
		if(targets_[i]->getStatus() == TargetTracking) {
#ifdef _DEBUG
			std::cout << i << " th target update" << std::endl;
#endif
			cv::Rect temp = targets_[i]->getRect(targets_[i]->getNumStates() - 1);
			ms_targets_[i].updateHistogram(image_color, temp, 1.0f, 0.8f);
		}
	}
#ifdef _DEBUG
	std::cout << "done" << std::endl;
#endif
}

void TargetManager::runMeanShift(cv::Mat &image_color)
{
	// clear buffer
	ms_rts_.clear();
	ms_sims_.clear();

	cv::Rect rt;
	float sim;

	my_assert(targets_.size() == ms_targets_.size());
	for(size_t i = 0; i < ms_targets_.size(); i++) {
		if(targets_[i]->getStatus() == TargetTracking) {
			// run meanshift
			sim = ms_targets_[i].runScaleMeanShift(image_color, rt);
			if(sim < 0.6) ms_targets_[i].resetModel();
			ms_sims_.push_back(sim);
			rt.height /= ms_targets_[i].getHeightRatio();
			ms_rts_.push_back(rt);
		}
	}
}

void TargetManager::setParameters(const std::string &name, const std::string &value)
{
	if (name == "match_dist_threshold") 	max_dist_ = boost::lexical_cast<double>(value);
	else if (name == "match_dist_overlap")	min_overlap_ = boost::lexical_cast<double>(value);
}

/*** function to compute the distance between a target and a detection.
 *
 *
 ***/
float TargetManager::computeDistance(const TargetPtr target, const ObjectStatePtr det, const cv::Mat &image)
{
	float dist = 0;
	
	dist = target->getDistance(det);
	if(dist > max_dist_)
		dist = INF_DIST;

	return dist;
}

float TargetManager::computeDistance(const TargetPtr target, const cv::Rect &det, const cv::Mat &image)
{
	float dist= 0;
	
	if(target->computeColorSim(image, det) < 0.65) {
		return INF_DIST;
	}

	dist = target->getOverlap(det);
	if(dist < min_overlap_) {
		dist = INF_DIST;
	}
	else {
		dist = -log(dist);
	}

	return dist;
}

void TargetManager::getProposals(const std::vector<cv::Rect> &dets, const cv::Mat &image, std::vector<cv::Rect> &proposals)
{
	std::vector<int>				association;
	std::vector<TargetPtr> 	trackingTargets;

	association.clear();
	proposals.clear();
	// find existing targets..
	for(size_t i = 0; i < targets_.size(); i++) {
		if(targets_[i]->getStatus() == TargetTracking) {
			trackingTargets.push_back(targets_[i]);
		}
	}
	// build assignment matrix
	if (trackingTargets.size() > 0) {
		float cost; 
		float *assign = new float [trackingTargets.size()];
		float *distMat = new float [trackingTargets.size() * dets.size()];

		for(size_t i = 0; i < trackingTargets.size(); i++) {
			for(size_t j = 0; j < dets.size(); j++) {
				distMat[i + trackingTargets.size() * j] = computeDistance(trackingTargets[i], dets[j], image);
			}
		}
		// solve assignment problem
		assignmentoptimal(assign, &cost, distMat, trackingTargets.size(), dets.size(), INF_DIST);

		// assign matched detections to proposal
		for(size_t i = 0; i < trackingTargets.size(); i++) {
			int detid = (int)assign[i];
			association.push_back(detid);
		}

		delete assign;
		delete distMat;
	}
	// handle remaining(non-matched) detections
	for(size_t i = 0; i < dets.size(); i++) {
		if(std::find(association.begin(), association.end(), i) == association.end())
			association.push_back((int)i);
	}
	// allocate proposals
	for(size_t i = 0; i < association.size(); i++)
	{
		cv::Rect rt(0, 0, 0, 0);
		if(association[i] >= 0) {
			rt = dets[association[i]];
		}
		proposals.push_back(rt);
	}

}

// arrange detections by solving association problem to targets.
void TargetManager::getProposals(const std::vector<ObjectStatePtr> &dets, const cv::Mat &image, std::vector<ObjectStatePtr> &proposals)
{
	std::vector<int>				association;
	std::vector<TargetPtr> 	trackingTargets;

	association.clear();
	proposals.clear();
	// find existing targets..
	for(size_t i = 0; i < targets_.size(); i++) {
		if(targets_[i]->getStatus() == TargetTracking) {
			trackingTargets.push_back(targets_[i]);
		}
	}
	// build assignment matrix
	if (trackingTargets.size() > 0) {
		float cost; 
		float *assign = new float [trackingTargets.size()];
		float *distMat = new float [trackingTargets.size() * dets.size()];

		for(size_t i = 0; i < trackingTargets.size(); i++) {
			for(size_t j = 0; j < dets.size(); j++) {
				distMat[i + trackingTargets.size() * j] = computeDistance(trackingTargets[i], dets[j], image);
			}
		}
		// solve assignment problem
		assignmentoptimal(assign, &cost, distMat, trackingTargets.size(), dets.size(), INF_DIST);

		// assign matched detections to proposal
		for(size_t i = 0; i < trackingTargets.size(); i++) {
			int detid = (int)assign[i];
			association.push_back(detid);
		}

		delete assign;
		delete distMat;
	}
	// handle remaining(non-matched) detections
	for(size_t i = 0; i < dets.size(); i++) {
		if(std::find(association.begin(), association.end(), i) == association.end())
			association.push_back((int)i);
	}
	// allocate proposals
	for(size_t i = 0; i < association.size(); i++)
	{
		ObjectStatePtr state;
		if(association[i] >= 0) {
			state = dets[association[i]];
#ifdef _DEBUG
			// debug message
			std::cout << "propose target " << i << " : confidence " << std::setprecision(5) << state->confidence_ << std::endl;
			std::cout << "\t loc : " << state->x_ << ", " << state->y_ << ", " << state->z_ << std::endl;
#endif
		}
		proposals.push_back(state);
	}
}

void TargetManager::saveAllTargets(std::string &dirname)
{
	int cnt = 0;
	for(size_t i = 0; i < targets_.size(); i++)
	{
		if(targets_[i]->getNumStates() == 1)
			continue;

		ostringstream filename;
		filename << dirname << "/target" << cnt++ << ".txt";
		targets_[i]->writeToFile(filename.str());
	}
}

void TargetManager::saveAllFeatures(std::string &dirname)
{
	int cnt = 0;
	for(size_t i = 0; i < feats_.size(); i++)
	{
		if(feats_[i]->getNumStates() == 1)
			continue;
		ostringstream filename;
		filename << dirname << "/feat" << cnt++ << ".txt";
		feats_[i]->writeToFile(filename.str());
	}
}

void TargetManager::saveAllCamera(std::string &dirname)
{
	ostringstream filename;
	filename << dirname << "/camera.txt";
	cam_.writeToFile(filename.str());
	// ostringstream filename2;
	// filename2 << dirname << "/gt_camera.txt";
	// gt_cam_.writeToFile(filename2.str());
}

void TargetManager::saveAll(std::string &dirname)
{
	saveAllCamera(dirname);
	saveAllFeatures(dirname);
	saveAllTargets(dirname);
	saveAllLines(dirname);
}

std::vector<int> TargetManager::getTerminatingTargets(double threshold, cv::Size im_size)
{
	std::vector<int> current_target_idx;
	std::vector<int> remove_idx;
	int counter = 0;
	
	// find id match to tracker.. sub-optimal way..
	for(size_t i = 0; i < targets_.size(); i++) {
		if(targets_[i]->getStatus() == TargetTracking) {
			current_target_idx.push_back(counter++);
		}
		else {
			current_target_idx.push_back(-1);
		}
	}

	// no target can be initiated outside of the image
	for(size_t i = 0; i < targets_.size(); i++) {
		if(targets_[i]->getStatus() == TargetTracking) {
			cv::Rect imrt(0, 0, im_size.width, im_size.height);
			cv::Rect trt = targets_[i]->getRect(targets_[i]->getNumStates() - 1);
			cv::Rect irt;

			trt.height /= 2;
			irt = trt & imrt;
			
			float visible = (float)(irt.width * irt.height) / (trt.width * trt.height);
			if(visible < 0.3) { // filter out
				targets_[i]->setStatus(TargetTerminated);
				remove_idx.push_back(current_target_idx[i]);
			}
			else if(visible < 0.8 && targets_[i]->getNumStates() < 3) {
				targets_[i]->setStatus(TargetTerminated);
				remove_idx.push_back(current_target_idx[i]);
			}
		}
	}

	// filter from confidence values
	for(size_t i = 0; i < targets_.size(); i++) {
		if(targets_[i]->getStatus() == TargetTracking) {
			ObjectStatePtr state = targets_[i]->getState(targets_[i]->getNumStates() - 1);
			if(state->getConfidence() < threshold) {
				targets_[i]->setStatus(TargetTerminated);
				remove_idx.push_back(current_target_idx[i]);
				// std::cout << "Terminating target " << i << " : confidence " << std::setprecision(5) << state->getConfidence() << std::endl;
				// std::cout << "\t loc : " << state->getX() << ", " << state->getY() << ", " << state->getZ() << std::endl;
			}
			else {
				// std::cout << "Tracking target " << i << " : confidence " << std::setprecision(5) << state->getConfidence() << std::endl;
				// std::cout << "\t loc : " << state->getX() << ", " << state->getY() << ", " << state->getZ() << std::endl;
			}
		}
	}
	// filter from ovelapping bounding box
	for(size_t i = 0; i < targets_.size(); i++) {
		if(targets_[i]->getStatus() == TargetTracking) {
			int idx1 = targets_[i]->getNumStates() - 1; // last item = current time frame
			cv::Rect r1 = targets_[i]->getRect(idx1);

			for(size_t j = i + 1; j < targets_.size(); j++) {
				if(targets_[j]->getStatus() == TargetTracking) {
					int idx2 = targets_[j]->getNumStates() - 1; // last item = current time frame
					cv::Rect r2 = targets_[j]->getRect(idx2);

					double ol = bb_overlap(r1, r2);
					if(ol > 0.65) {
						if(idx1 > idx2) { // target i is tracked for longer time
							targets_[j]->setStatus(TargetTerminated);
							remove_idx.push_back(current_target_idx[j]);
						}
						else {
							targets_[i]->setStatus(TargetTerminated);
							remove_idx.push_back(current_target_idx[i]);
							break;
						}
					}
					else {
						float dist  = abs((r1.x + r1.width / 2) - (r2.x + r2.width / 2));
						if((dist < r1.width / 4) || (dist < r2.width / 4)){
							cv::Rect ir = r1 & r2;
							float covered = (float)(ir.width * ir.height) / (r1.width * r1.height);
							if(covered > 0.9) {
								targets_[i]->setStatus(TargetTerminated);
								remove_idx.push_back(current_target_idx[i]);
								break;
							}
							covered = (float)(ir.width * ir.height) / (r2.width * r2.height);
							if(covered > 0.9) {
								targets_[j]->setStatus(TargetTerminated);
								remove_idx.push_back(current_target_idx[j]);
							}
						}
					}
				}
			}
		}
	}
	// sort
	std::sort(remove_idx.begin(), remove_idx.end());
	return remove_idx;
}

cv::Scalar TargetManager::get_target_color(int id)
{
	cv::Scalar color = cv::Scalar(((id * 120) % 256), ((id * 60) % 256), ((id * 30) % 256));
	return color;
}

cv::Mat TargetManager::drawFeatures(cv::Mat &image, const std::vector<int> &remove_idx, double ts)
{
	cv::Mat image_draw = image.clone();

	int idx = 0;
	for(size_t i = 0; i < feats_.size(); i++)
	{
		if(feats_[i]->getNumStates() > 0) {
			int cur_idx = feats_[i]->findIdx(ts);
			if(cur_idx < 0) { continue; }

			cv::Scalar col = cv::Scalar(155, 0, 155);
			if(std::find(remove_idx.begin(), remove_idx.end(), idx) != remove_idx.end()) { // removed
				col = cv::Scalar(255, 255, 255);
			}
			else if(cur_idx == 0) { // new_feature
				col = cv::Scalar(0, 0, 0);
			}
			else {
			}

			cv::Rect rt;
			rt.x = feats_[i]->getObs(cur_idx).x - 5;
			rt.y = feats_[i]->getObs(cur_idx).y - 5;
			rt.width = 10; rt.height = 10;

			cv::rectangle(image_draw, rt.tl(), rt.br(), col, 2);
			for(int j = max(0, cur_idx - 10); j < cur_idx; j++) {
				cv::line(image_draw, feats_[i]->getObs(j), feats_[i]->getObs(j + 1), cv::Scalar(155, 0, 155));
			}

			idx++;
		}
	}
	return image_draw;
}

cv::Mat TargetManager::drawTargets(cv::Mat &image_color, double ts)
{
	cv::Mat image_draw = image_color.clone();

	int idx = 0;
	for(size_t i = 0; i < targets_.size(); i++)
	{
		if(targets_[i]->getNumStates() > 0) {
			if(targets_[i]->getStatus() == TargetTracking) {
				int state_idx = targets_[i]->findIdx(ts);
				if(state_idx < 0)  { 
					for(size_t k = 0; k < targets_[i]->states_.size(); k++) {
						targets_[i]->states_[k]->print();
					}
					std::cout << std::endl;
					std::cout << "Target[" << i << "," << idx << "] doesnot have state at " << setprecision(5) << ts - floor(ts / 1000) * 1000 << std::endl;
					idx++;
					assert(0);
				}
				
				cv::Rect rt = targets_[i]->getRect(state_idx);
				double width = floor(image_draw.cols / 100 * targets_[i]->getState(state_idx)->getConfidence());
				cv::Scalar color = get_target_color(idx);

				cv::rectangle(image_draw, rt.tl(), rt.br(), color, width);

				ostringstream text;
				text << "T" << idx << ", " << std::setprecision(2) << targets_[i]->getState(state_idx)->getConfidence();
				rt.y -= image_draw.cols / 100;
				cv::putText(image_draw, text.str(), rt.tl(), cv::FONT_HERSHEY_SIMPLEX, .4 * image_draw.cols / 500, color, image_draw.cols / 500);
			}
			idx++;
		}
	}
	
	idx = findLines(ts);
	if(idx >= 0) {
		std::vector<longline> lines = lines_[idx].getLines();
		for(size_t k = 0; k < lines.size(); k++) {
			cv::line(image_draw, cv::Point(lines[k].x1_, lines[k].y1_), cv::Point(lines[k].x2_, lines[k].y2_), cv::Scalar(0, 150, 255), image_draw.cols / 250);
		}

		lines = lines_[idx].getLanes();
		for(size_t k = 0; k < lines.size(); k++) {
			cv::line(image_draw, cv::Point(lines[k].x1_, lines[k].y1_), cv::Point(lines[k].x2_, lines[k].y2_), cv::Scalar(219, 112, 147), image_draw.cols / 100);
		}
	}

	return image_draw;
}

int TargetManager::findLines(double timesec)
{
	for(int i = 0; i < lines_.size(); i++) {
		if(timesec == lines_[i].getTimesec()) {
			return i;
		}
	}

	return -1;
}

void TargetManager::updateLines(const std::vector<longline> &lines, const std::vector<longline> &lanes, double timestamp)
{
	int idx = findLines(timestamp);
	if(idx < 0) {
		Lines line(timestamp);
		line.setLines(lines);
		line.setLanes(lanes);
		lines_.push_back(line);
	}
	else {
		lines_[idx].setLines(lines);
		lines_[idx].setLanes(lanes);
	}
}

void TargetManager::saveAllLines(std::string &dirname)
{
	std::string filename = dirname + "/lines.txt";

	std::ofstream out(filename.c_str()); 
	if(!out) { 
		cout << "TargetManager::saveAllLines - Cannot open file " << filename << std::endl; 
		assert(0);
	}
	for(size_t i = 0; i < lines_.size(); i++) {
		lines_[i].writeLinesToFile(out);
	}
	out.close();

	filename = dirname + "/lanes.txt";
	std::ofstream out2(filename.c_str());
	if(!out2) { 
		cout << "TargetManager::saveAllLines - Cannot open file " << filename << std::endl; 
		assert(0);
	}
	for(size_t i = 0; i < lines_.size(); i++) {
		lines_[i].writeLanesToFile(out2);
	}
	out2.close();
}
