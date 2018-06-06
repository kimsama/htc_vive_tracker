#include "htc_vive_tracker.h"

CHtc_Vive_Tracker::CHtc_Vive_Tracker()
{

}
 
CHtc_Vive_Tracker::~CHtc_Vive_Tracker()
{
	this->ShutDownVR(false);
}

void CHtc_Vive_Tracker::InitializeDeviceMap(bool verbose){
	int num_detected_devices = max_devices_;
	if (verbose) std::cout<<"Detected devices:"<<std::endl;	
	vr::TrackedPropertyError peError = vr::TrackedProp_Success;
	std::string device_name;
	for (int i = 0; i<num_detected_devices; ++i){
		if (device_poses_[i].bDeviceIsConnected && device_poses_[i].bPoseIsValid){
			device_name = this->SetDeviceName(i);
			if (verbose) std::cout<<i<<" :  "<<device_name<<std::endl;
			devices_id_[device_name] = i;
			devices_names_[i] = device_name;
		}
	}
}

// HtcViveTrackerAlgorithm Public API
//Initialize and shutdown functionalities
bool CHtc_Vive_Tracker::InitializeVR(bool verbose){

        bool runtime_ok  = vr::VR_IsRuntimeInstalled();
        bool hmd_present = vr::VR_IsHmdPresent();
        vr::EVRInitError er;
        this->vr_system_ = vr::VR_Init (&er, vr::VRApplication_Background);
        std::string init_error = vr::VR_GetVRInitErrorAsSymbol(er);

        if (verbose){
                std::cout<<"VR is runtime installed : " <<runtime_ok<<std::endl;
                std::cout<<"VR is HMD present : " <<hmd_present<<std::endl;
                std::cout<<"VR init error : " <<er<<init_error<<std::endl;
        }

        if (runtime_ok && hmd_present && er==vr::VRInitError_None) {

		this->max_devices_ = vr::k_unMaxTrackedDeviceCount;
		for (int i = 0; i < this->max_devices_; ++i){
			devices_names_.push_back("");
		}

		this->Update(verbose);
	 	this->InitializeDeviceMap(verbose);
		return true;
        }
	else return false;


	
}

bool CHtc_Vive_Tracker::ShutDownVR(bool verbose){
 	if (this->vr_system_){
      		vr::VR_Shutdown();
	if (verbose){
		std::cout<<"Device is shut down"<<std::endl;
	}
		return true;
	} 
	else {
		return false;
	}
	
	
}
 

void CHtc_Vive_Tracker::Update (bool verbose){
	this->vr_system_->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, device_poses_, max_devices_);
	for (vr::TrackedDeviceIndex_t device_index  = vr::k_unTrackedDeviceIndex_Hmd; device_index < max_devices_; ++device_index){
		if (device_poses_[device_index].bDeviceIsConnected && device_poses_[device_index].bPoseIsValid){
			if (verbose) {
				std::string info = ("device["+std::to_string(device_index)+"]: " + this->GetDeviceClass(device_index) + " " + std::to_string (device_poses_[device_index].eTrackingResult));
				std::cout<<info<<std::endl;
			}
		}

	}
}

//Device detection
bool CHtc_Vive_Tracker::IsDeviceDetected (const std::string & device_name){
	if (devices_id_.find(device_name) == devices_id_.end()){
		return false;
	}
	int device_index = devices_id_[device_name];
	if (device_index < max_devices_){
		return this->vr_system_->IsTrackedDeviceConnected(device_index);
	}
	else return false;
}
void CHtc_Vive_Tracker::PrintAllDetectedDevices (){
	for (vr::TrackedDeviceIndex_t device_index  = vr::k_unTrackedDeviceIndex_Hmd; device_index < max_devices_; ++device_index){
		if (device_poses_[device_index].bDeviceIsConnected && device_poses_[device_index].bPoseIsValid){

			std::string device_name = devices_names_[device_index];
			std::string info = ("device["+std::to_string(device_index)+"]: " + device_name + " is connected");
			std::cout<<info<<std::endl;
		}
	}
}


//Device position 
bool CHtc_Vive_Tracker::GetDevicePoseQuaternion (const std::string & device_name,double (&pose)[3], double (&angle)[4]){
	if (devices_id_.find(device_name) == devices_id_.end()){
		return false;
	}
	int device_index = devices_id_[device_name];
	this->vr_system_->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, device_poses_, max_devices_);
	vr::TrackedDevicePose_t current_device_pose = device_poses_[device_index];
	if (current_device_pose.bDeviceIsConnected && current_device_pose.bPoseIsValid){
			vr::HmdMatrix34_t device_matrix = current_device_pose.mDeviceToAbsoluteTracking;
			this->MatrixToPose(device_matrix,pose);
			this->MatrixToQuaternion(device_matrix,angle);
			std::string info = std::to_string (current_device_pose.eTrackingResult);
			return true;
	}
	return false;

}
    bool CHtc_Vive_Tracker::GetDevicePoseEuler (const std::string & device_name, double (&pose)[3], double & roll, double & pitch, double &yaw){

	double quaternion[4];
	if (this->GetDevicePoseQuaternion(device_name,pose,quaternion)){
		double sinr = 2 * (quaternion[3]*quaternion[0] + quaternion[1]*quaternion[2]);
		double cosr = 1 - (2*(quaternion[0]*quaternion[0]+quaternion[1]*quaternion[1]));

		roll = atan2 (sinr,cosr);

		double sinp = 2 * (quaternion[3]*quaternion[1] - quaternion[2]*quaternion[0]);
		if (fabs(sinp)>=1){
			pitch = copysign (M_PI/2, sinp);
		} else {
      			pitch = asin (sinp);
		}
		
		double siny = 2 * (quaternion[4]*quaternion[3] + quaternion[0]*quaternion[1]);
		double cosy = 1 - (2*(quaternion[1]*quaternion[1] + quaternion[2]*quaternion[2]));
		yaw = atan2(siny,cosy);
		return true;
	}	

	return false;
}
bool CHtc_Vive_Tracker::GetDeviceVelocity (const std::string & device_name, double (&linear_velocity)[3], double (&angular_velocity)[3]){
	this->vr_system_->GetDeviceToAbsoluteTrackingPose(vr::TrackingUniverseStanding, 0, device_poses_, max_devices_);
	int device_index = devices_id_[device_name];
	if (device_index < this->max_devices_){
		if (device_poses_[device_index].bDeviceIsConnected && device_poses_[device_index].eTrackingResult == vr::TrackingResult_Running_OK){
			for (int i = 0; i < 3; ++i){
				linear_velocity[i] = device_poses_[device_index].vVelocity.v[i];
				angular_velocity[i] = device_poses_[device_index].vAngularVelocity.v[i];
			}
		return true;
	}
	}
	return false;
}


//https://github.com/osudrl/CassieVrControls/wiki/OpenVR-Quick-Start
std::string CHtc_Vive_Tracker::GetDeviceClass (const int device_id){
    vr::ETrackedDeviceClass tracked_device_class = this->vr_system_ ->GetTrackedDeviceClass (device_id);
    if (tracked_device_class == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller){
	return NAME_CONTROLLER;
    } else if (tracked_device_class == vr::ETrackedDeviceClass::TrackedDeviceClass_HMD){
	return NAME_HMD;
    } else if (tracked_device_class == vr::ETrackedDeviceClass::TrackedDeviceClass_TrackingReference){
	return NAME_TREFERENCE;
    } else if (tracked_device_class == vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker){
    	return NAME_TRACKER;
    } else if (tracked_device_class == vr::ETrackedDeviceClass::TrackedDeviceClass_Invalid){
	return NAME_NULL;
    } else return NAME_NULL;
}

std::string CHtc_Vive_Tracker::SetDeviceName (const int device_id){
	std::string class_name = this->GetDeviceClass (device_id);
	std::string device_name;
	if (class_name == NAME_CONTROLLER){
		device_name = class_name+"_"+std::to_string(controller_counts_);
		controller_counts_++;
		
	} else if (class_name == NAME_HMD) {
		device_name = class_name+"_"+std::to_string(hmd_counts_);
		hmd_counts_++;
	} else if (class_name == NAME_TREFERENCE) {
		device_name = class_name+"_"+std::to_string(track_reference_counts_);
		track_reference_counts_++;
	} else if (class_name == NAME_TRACKER){
		device_name = class_name+"_"+ std::to_string (tracker_counts_);
		tracker_counts_++;
	} else if (class_name == NAME_NULL){
		device_name = class_name +"_"+ std::to_string (null_counts_);
		null_counts_++;
	} else device_name = NAME_NULL;
	
	return device_name;

}


//https://github.com/osudrl/CassieVrControls/wiki/OpenVR-Quick-Start              
void CHtc_Vive_Tracker::MatrixToPose(const vr::HmdMatrix34_t & matrix,double (&pose)[3]){
	pose[0] = matrix.m[0][3];
	pose[1] = matrix.m[1][3];
	pose[2] = matrix.m[2][3];

}
void CHtc_Vive_Tracker::MatrixToQuaternion(const vr::HmdMatrix34_t & matrix,double (&quaternion)[4]){
	//w
	quaternion[3] = sqrt(fmax(0,1+matrix.m[0][0] + matrix.m[1][1] + matrix.m[2][2]))/2;

	//x
	quaternion[0] = sqrt(fmax(0, 1 + matrix.m[0][0] - matrix.m[1][1] - matrix.m[2][2])) / 2;
	
	//y
	quaternion[1] = sqrt(fmax(0, 1 - matrix.m[0][0] + matrix.m[1][1] - matrix.m[2][2])) / 2;
	
	//z
	quaternion[2] = sqrt(fmax(0, 1 - matrix.m[0][0] - matrix.m[1][1] + matrix.m[2][2])) / 2;
	quaternion[0] = copysign(quaternion[0], matrix.m[2][1] - matrix.m[1][2]);
	quaternion[1] = copysign(quaternion[1], matrix.m[0][2] - matrix.m[2][0]);
	quaternion[2] = copysign(quaternion[2], matrix.m[1][0] - matrix.m[0][1]);
}
