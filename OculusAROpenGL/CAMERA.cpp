#ifndef _CAMERA__
#define _CAMERA__
#include <ovrvision.h>        //Ovrvision SDK
#include "HMD.cpp"
#include <string>
#include <sstream>
#include <iostream>

class CAMERA{
private:
	unsigned char jpegImageL[100000];
	unsigned char jpegImageR[100000];
	FILE *pR;
	FILE *pL;
	int imageNumber;
	OVR::Ovrvision* g_pOvrvision;
	int processer_quality;
	int imageSize;
public:
	CAMERA(HMD* hmd, bool isDebug) {
		g_pOvrvision = new OVR::Ovrvision();
		imageNumber = 0;
		g_pOvrvision->DefaultSetting();
		int processer_quality = OVR::OV_PSQT_HIGH;

		if (hmd->getDevice()->Type == ovrHmd_DK2 || isDebug) {
			//Rift DK2
			g_pOvrvision->Open(0, OVR::OV_CAMVGA_FULL);  //Open
			imageSize = 3 * 640 * 480;
		}
		else {
			//Rift DK1
			g_pOvrvision->Open(0, OVR::OV_CAMVGA_FULL, OVR::OV_HMD_OCULUS_DK1);  //Open
		}
	}
	OVR::Ovrvision* getDevice(){
		return g_pOvrvision;

	}
	void preStoreCamData() {
		g_pOvrvision->PreStoreCamData();	//renderer
	}
	unsigned char* getCamImageLeft() {
		return g_pOvrvision->GetCamImage(OVR::OV_CAMEYE_LEFT, (OvPSQuality)processer_quality);
	}
	unsigned char* getCamImageRight() {
		return g_pOvrvision->GetCamImage(OVR::OV_CAMEYE_RIGHT, (OvPSQuality)processer_quality);
	}

	void storeCamImage(int eye, const char* filePath) {
		
		pL = fopen(filePath, "wb");
		if (eye == OVR::OV_CAMEYE_LEFT){
			g_pOvrvision->GetCamImageMJPEG(jpegImageL, &imageSize, OVR::OV_CAMEYE_LEFT);
			fwrite(jpegImageL, 1, imageSize, pL);
		}
		else if (eye == OVR::OV_CAMEYE_RIGHT){
			g_pOvrvision->GetCamImageMJPEG(jpegImageL, &imageSize, OVR::OV_CAMEYE_RIGHT);
			fwrite(jpegImageL, 1, imageSize, pL);
		}
		else
			printf("*** ERROR *** Wrong choice of camera");
		
		fclose(pL);
	}
	void storeCamImages() {
		//char * string = "image	" 
		char numberChar[10];
		char filePath[100] = "./CamData/image"; //TODO:Rustam can I make it variable length?

		sprintf(numberChar, "%d", imageNumber);
		strcat(filePath, numberChar);
		strcat(filePath, "L.jpeg");				
		pL = fopen(filePath, "wb");
		g_pOvrvision->GetCamImageMJPEG(jpegImageL, &imageSize, OVR::OV_CAMEYE_LEFT);
		fwrite(jpegImageL, 1, imageSize, pL);
		fclose(pL);

		strcpy(filePath, "./CamData/image");
		strcat(filePath, numberChar);
		strcat(filePath, "R.jpeg");
		pR = fopen(filePath, "wb");
		g_pOvrvision->GetCamImageMJPEG(jpegImageR, &imageSize, OVR::OV_CAMEYE_RIGHT);
		fwrite(jpegImageR, 1, imageSize, pR);
		fclose(pR);
		imageNumber++;
		//delete[] filePath;
		//delete[] numberChar;
	}

	void cleanup()
	{
		
	}


};
#endif