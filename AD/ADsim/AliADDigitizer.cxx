/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/
 
/* $Id: AliADDigitizer.cxx  $ */

///_________________________________________________________________________
///
/// This class constructs Digits out of Hits
///
///

// --- Standard library ---

// --- ROOT system ---
#include <TMath.h>
#include <TTree.h>
#include <TMap.h>
#include <TGeoManager.h>
#include <TGeoPhysicalNode.h>
#include <AliGeomManager.h>
#include <TRandom.h>
#include <TF1.h>
#include <TH1F.h>
#include <TH2F.h>

// --- AliRoot header files ---
#include "AliRun.h"
#include "AliDetector.h"
#include "AliAD.h"
#include "AliADhit.h"
#include "AliADConst.h"
#include "AliRunLoader.h"
#include "AliLoader.h"
#include "AliGRPObject.h"
#include "AliDigitizationInput.h"
#include "AliCDBManager.h"
#include "AliCDBStorage.h"
#include "AliCDBEntry.h"
#include "AliADCalibData.h"
#include "AliCTPTimeParams.h"
#include "AliLHCClockPhase.h"
#include "AliADdigit.h"
#include "AliADDigitizer.h"
#include "AliADSDigit.h"
#include "AliADTriggerSimulator.h"

ClassImp(AliADDigitizer)

//____________________________________________________________________________
 AliADDigitizer::AliADDigitizer()
                   :AliDigitizer(),
                    fCalibData(GetCalibData()),
                    fNdigits(0),
                    fDigits(0),
                    fTimeSignalShape(NULL),
                    fChargeSignalShape(NULL),
		    fEvenOrOdd(kFALSE),
		    fTask(kHits2Digits),
		    fAD(NULL)
{
  // default constructor
  // Initialize OCDB and containers used in the digitization

  Init();
}

//____________________________________________________________________________ 
  AliADDigitizer::AliADDigitizer(AliAD *AD, DigiTask_t task)
                    :AliDigitizer(),
		     fCalibData(GetCalibData()),
		     fNdigits(0),
                     fDigits(0),
		     fTimeSignalShape(NULL),
                     fChargeSignalShape(NULL),
		     fEvenOrOdd(kFALSE),
		     fTask(task),
		     fAD(AD)
{
  // constructor
  // Initialize OCDB and containers used in the digitization

  Init();
}
           
//____________________________________________________________________________ 
  AliADDigitizer::AliADDigitizer(AliDigitizationInput* digInput)
                    :AliDigitizer(digInput),
		     fCalibData(GetCalibData()),
		     fNdigits(0),
                     fDigits(0),
		     fTimeSignalShape(NULL),
                     fChargeSignalShape(NULL),
		     fEvenOrOdd(kFALSE),
		     fTask(kHits2Digits),
		     fAD(NULL)
{
  // constructor
  // Initialize OCDB and containers used in the digitization

  Init();
}
           
//____________________________________________________________________________ 
  AliADDigitizer::~AliADDigitizer()
{
  // destructor
  
  if (fDigits) {
    fDigits->Delete();
    delete fDigits;
    fDigits=0; 
  }

  if (fTimeSignalShape) {
    delete fTimeSignalShape;
    fTimeSignalShape = NULL;
  }
  if (fChargeSignalShape) {
    delete fChargeSignalShape;
    fChargeSignalShape = NULL;
  }

  for(Int_t i = 0 ; i < 16; ++i) {
    if (fTime[i]) delete [] fTime[i];
  }
}

//____________________________________________________________________________ 
Bool_t AliADDigitizer::Init()
{
  // Initialises the digitizer
  // Initialize OCDB and containers used in the digitization

  // check if the digitizer was already initialized
  if (fTimeSignalShape) return kTRUE;
  fTimeSignalShape = new TF1("ADTimeSignalShape",this,&AliADDigitizer::TimeSignalShape,0,200,6,"AliADDigitizer","TimeSignalShape");
  fChargeSignalShape = new TF1("ADChargeSignalShape",this,&AliADDigitizer::ChargeSignalShape,0,140,3,"AliADDigitizer","ChargeSignalShape");

  fTimeSignalShape->SetParameters(-1.07335e+00,2.16002e+01,-1.26133e-01,
			           1.41619e+00,5.50334e-01,3.86111e-01);

  // Now get the CTP L0->L1 delay
  AliCDBEntry *entry = AliCDBManager::Instance()->Get("GRP/CTP/CTPtiming");
  if (!entry) AliFatal("CTP timing parameters are not found in OCDB !");
  AliCTPTimeParams *ctpParams = (AliCTPTimeParams*)entry->GetObject();
  Float_t l1Delay = (Float_t)ctpParams->GetDelayL1L0()*25.0;

  AliCDBEntry *entry1 = AliCDBManager::Instance()->Get("GRP/CTP/TimeAlign");
  if (!entry1) AliFatal("CTP time-alignment is not found in OCDB !");
  AliCTPTimeParams *ctpTimeAlign = (AliCTPTimeParams*)entry1->GetObject();
  l1Delay += ((Float_t)ctpTimeAlign->GetDelayL1L0()*25.0);

  AliCDBEntry *entry2 = AliCDBManager::Instance()->Get("AD/Calib/TimeDelays");
  if (!entry2) AliFatal("AD time delays are not found in OCDB !");
  TH1F *TimeDelays = (TH1F*)entry2->GetObject();

  AliCDBEntry *entry3 = AliCDBManager::Instance()->Get("GRP/Calib/LHCClockPhase");
  if (!entry3) AliFatal("LHC clock-phase shift is not found in OCDB !");
  AliLHCClockPhase *phase = (AliLHCClockPhase*)entry3->GetObject();
  
  //Get Pulse shape parameters
  AliCDBEntry *entry4 = AliCDBManager::Instance()->Get("AD/Calib/PulseShapes");
  if (!entry4) AliFatal("AD pulse shapes are not found in OCDB !");
  TH2F *PulseShapes = (TH2F*)entry4->GetObject();

  for(Int_t i = 0 ; i < 16; ++i) {
  
    fCssOffset[i] = PulseShapes->GetBinContent(i+1,1);
    fCssTau[i] = PulseShapes->GetBinContent(i+1,2);
    fCssSigma[i] = PulseShapes->GetBinContent(i+1,3);

    for(Int_t j = 0; j < kNClocks; ++j) fAdc[i][j] = 0;
    fLeadingTime[i] = fTimeWidth[i] = 0;

    fPmGain[i] = fCalibData->GetGain(i);

    fAdcPedestal[i][0] = fCalibData->GetPedestal(i);
    fAdcSigma[i][0]    = fCalibData->GetSigma(i); 
    fAdcPedestal[i][1] = fCalibData->GetPedestal(i+16);
    fAdcSigma[i][1]    = fCalibData->GetSigma(i+16); 

    Int_t board = AliADCalibData::GetBoardNumber(i);
    fNBins[i] = TMath::Nint(((Float_t)(fCalibData->GetMatchWindow(board)+1)*25.0+
			     (Float_t)kMaxTDCWidth*fCalibData->GetWidthResolution(board))/
			    fCalibData->GetTimeResolution(board));
    fNBinsLT[i] = TMath::Nint(((Float_t)(fCalibData->GetMatchWindow(board)+1)*25.0)/
			      fCalibData->GetTimeResolution(board));
    fBinSize[i] = fCalibData->GetTimeResolution(board);
    
    fHptdcOffset[i] = (((Float_t)fCalibData->GetRollOver(board)-
			(Float_t)fCalibData->GetTriggerCountOffset(board))*25.0
		       +fCalibData->GetTimeOffset(i)
		       -l1Delay
		       -phase->GetMeanPhase()
		       -TimeDelays->GetBinContent(i+1)
		       -kADOffset);
    	       
    fClockOffset[i] = (((Float_t)fCalibData->GetRollOver(board)-
			(Float_t)fCalibData->GetTriggerCountOffset(board))*25.0
		       +fCalibData->GetTimeOffset(i)
		       -l1Delay
		       -kADOffset);

    fTime[i] = new Float_t[fNBins[i]];
    memset(fTime[i],0,fNBins[i]*sizeof(Float_t));
    
    //std::cout<<"AD: "<<" fNBins = "<<fNBins[i]<<" fNBinsLT = "<<fNBinsLT[i]<<" fHptdcOffset = "<<fHptdcOffset[i]<<" fClockOffset = "<<fClockOffset[i]<<std::endl;  
  }
  
  return kTRUE;

}

//____________________________________________________________________________ 
void AliADDigitizer::Digitize(Option_t* /*option*/) 
{   
   // Creates digits from hits
  fNdigits = 0;  

  if (fAD && !fDigInput) {
    AliLoader *loader = fAD->GetLoader();
    if (!loader) {
      AliError("Can not get AD Loader via AliAD object!");
      return;
    }
    AliRunLoader* runLoader = AliRunLoader::Instance();
    for (Int_t iEvent = 0; iEvent < runLoader->GetNumberOfEvents(); ++iEvent) {
      runLoader->GetEvent(iEvent);
      if (fTask == kHits2Digits) {
	DigitizeHits();
	DigitizeSDigits();
	WriteDigits(loader);
      }
      else {
	DigitizeHits();
	WriteSDigits(loader);
      }
    }
  }
  else if (fDigInput) {
      ReadSDigits();
      DigitizeSDigits();
      AliRunLoader *currentLoader = AliRunLoader::GetRunLoader(fDigInput->GetOutputFolderName());
      AliLoader *loader = currentLoader->GetLoader("ADLoader");
      if (!loader) { 
	AliError("Cannot get AD Loader via RunDigitizer!");
	return;
      }
      WriteDigits(loader);
  }
  else {
    AliFatal("Invalid digitization task! Exiting!");
  }
}

//____________________________________________________________________________ 
void AliADDigitizer::DigitizeHits()
{
  // Digitize the hits to the level of
  // SDigits (fTime arrays)
  Int_t nTotPhot[16];
  Float_t PMmeanTime[16];
  Int_t nPMHits[16];
  
  for(Int_t i = 0 ; i < 16; ++i) {
    memset(fTime[i],0,fNBins[i]*sizeof(Float_t));
    fLabels[i][0] = fLabels[i][1] = fLabels[i][2] = -1;
    nTotPhot[i] = 0;
    PMmeanTime[i] = 0;
    nPMHits[i] = 0;
  }
  
  AliLoader* loader = fAD->GetLoader();
  if (!loader) {
     AliError("Can not get AD Loader!");
     return;
     }
  loader->LoadHits();
  TTree* treeH = loader->TreeH();
  if (!treeH) {
     AliError("Cannot get TreeH!");
     return;
     }
  TClonesArray* hits = fAD->Hits();

  //Loop over hits
  Int_t nTracks = (Int_t) treeH->GetEntries();
  for(Int_t iTrack = 0; iTrack < nTracks; iTrack++) {
     fAD->ResetHits();
     treeH->GetEvent(iTrack);
     Int_t nHits = hits->GetEntriesFast();
       for (Int_t iHit = 0; iHit < nHits; iHit++) {
	 AliADhit* hit = (AliADhit *)hits->UncheckedAt(iHit);
	 Int_t nPhot = hit->GetNphot();
	 Int_t pmt  = hit->GetCell();                          
	 if (pmt < 0) continue;
	 Int_t trackLabel = hit->GetTrack();
	 for(Int_t l = 0; l < 3; ++l) {
	   if (fLabels[pmt][l] < 0) {
	     fLabels[pmt][l] = trackLabel;
	     break;
	   }
	 }
	 Float_t dt_scintillator = gRandom->Gaus(0,kIntTimeRes);
	 Float_t t = dt_scintillator + hit->GetTof();
	 nTotPhot[pmt] += nPhot;
	 PMmeanTime[pmt] += t*nPhot;
	 nPMHits[pmt]++;
	 }//hit loop
     }//track loop
     
  //Now makes SDigits from hits
  for(Int_t iPM = 0; iPM < 16; iPM++) {
     if(nPMHits[iPM]==0)continue;
     if(nTotPhot[iPM]==0)continue;
     PMmeanTime[iPM] = PMmeanTime[iPM]/nTotPhot[iPM];
     PMmeanTime[iPM] += fHptdcOffset[iPM]; 
     
     fChargeSignalShape->SetParameters(fCssOffset[iPM],fCssTau[iPM],fCssSigma[iPM]);
     Float_t integral = fChargeSignalShape->Integral(0,140);
     //std::cout<<"Integral = "<<integral<<std::endl; 
      
     Float_t charge = nTotPhot[iPM]*fPmGain[iPM]*fBinSize[iPM]/integral;
	     
     Int_t firstBin = TMath::Max(0,(Int_t)((PMmeanTime[iPM])/fBinSize[iPM]));
     Int_t lastBin = fNBins[iPM]-1;
     //std::cout<<"First Bin: "<<firstBin<<std::endl;
     for(Int_t iBin = firstBin; iBin <= lastBin; ++iBin) {
	 Float_t tempT = fBinSize[iPM]*(0.5+iBin)-PMmeanTime[iPM];
	 if(tempT<=0)continue;
	 fTime[iPM][iBin] += charge*fChargeSignalShape->Eval(tempT);
	 }
     }//PM loop
     loader->UnloadHits();
}

//____________________________________________________________________________ 
void AliADDigitizer::DigitizeSDigits()
{
  // Digitize the fTime arrays (SDigits) to the level of
  // Digits (fAdc arrays)
  for(Int_t i = 0 ; i < 16; ++i) {
    for(Int_t j = 0; j < kNClocks; ++j) fAdc[i][j] = 0;
    fLeadingTime[i] = fTimeWidth[i] = 0;
  }


  for (Int_t ipmt = 0; ipmt < 16; ++ipmt) {
  
    fChargeSignalShape->SetParameters(fCssOffset[ipmt],fCssTau[ipmt],fCssSigma[ipmt]);
    Float_t maximum = 0.9*fChargeSignalShape->GetMaximum(0,140); 
    Float_t integral = fChargeSignalShape->Integral(0,140);
    Float_t thr = fCalibData->GetCalibDiscriThr(ipmt)*kChargePerADC*maximum*fBinSize[ipmt]/integral;
    //Float_t thr = 0;
       
    Bool_t ltFound = kFALSE, ttFound = kFALSE;
    for (Int_t iBin = 0; iBin < fNBins[ipmt]; ++iBin) {
      Float_t t = fBinSize[ipmt]*Float_t(iBin);
      if (fTime[ipmt][iBin] > thr) {
	if (!ltFound && (iBin < fNBinsLT[ipmt])) {
	  ltFound = kTRUE;
	  fLeadingTime[ipmt] = t;
	  //std::cout<<"Leading Bin: "<<iBin<<std::endl;
	  //std::cout<<"Leading TADC: "<<t-fClockOffset[ipmt]<<std::endl;
	}
      }
      else {
	if (ltFound) {
	  if (!ttFound) {
	    ttFound = kTRUE;
	    fTimeWidth[ipmt] = t - fLeadingTime[ipmt];
	  }
	}
      }
      Float_t tadc = t - fClockOffset[ipmt];
      Int_t clock = kNClocks/2 + Int_t(tadc/25.0);
      if (clock >= 0 && clock < kNClocks)
	fAdc[ipmt][clock] += fTime[ipmt][iBin]/kChargePerADC;
    }
    AliDebug(1,Form("Channel %d Offset %f Time %f",ipmt,fClockOffset[ipmt],fLeadingTime[ipmt]));
    Int_t board = AliADCalibData::GetBoardNumber(ipmt);
    if (ltFound && ttFound) {
      fTimeWidth[ipmt] = fCalibData->GetWidthResolution(board)*
	Float_t(Int_t(fTimeWidth[ipmt]/fCalibData->GetWidthResolution(board)));
      if (fTimeWidth[ipmt] < Float_t(kMinTDCWidth)*fCalibData->GetWidthResolution(board))
	fTimeWidth[ipmt] = Float_t(kMinTDCWidth)*fCalibData->GetWidthResolution(board);
      if (fTimeWidth[ipmt] > Float_t(kMaxTDCWidth)*fCalibData->GetWidthResolution(board))
	fTimeWidth[ipmt] = Float_t(kMaxTDCWidth)*fCalibData->GetWidthResolution(board);
    }
  }

  fEvenOrOdd = gRandom->Integer(2);
  for (Int_t j=0; j<16; ++j){
    for (Int_t iClock = 0; iClock < kNClocks; ++iClock) {
      Int_t integrator = (iClock + fEvenOrOdd) % 2;
      AliDebug(1,Form("ADC %d %d %f",j,iClock,fAdc[j][iClock]));
      fAdc[j][iClock]  += gRandom->Gaus(fAdcPedestal[j][integrator], fAdcSigma[j][integrator]);
    }
  }
  //Fill BB and BG flags in trigger simulator
  AliADTriggerSimulator * triggerSimulator = new AliADTriggerSimulator();
  triggerSimulator->FillFlags(fBBFlag,fBGFlag,fLeadingTime);
 	
}

//____________________________________________________________________________ 
void AliADDigitizer::ReadSDigits()
{
  // Read SDigits which are then to precessed
  // in the following method
  for(Int_t i = 0 ; i < 16; ++i) {
    memset(fTime[i],0,fNBins[i]*sizeof(Float_t));
    fLabels[i][0] = fLabels[i][1] = fLabels[i][2] = -1;
  }

  // Loop over input files
  Int_t nFiles= fDigInput->GetNinputs();
  for (Int_t inputFile = 0; inputFile < nFiles; inputFile++) {
    // Get the current loader 
    AliRunLoader* currentLoader = 
      AliRunLoader::GetRunLoader(fDigInput->GetInputFolderName(inputFile));

    AliLoader *loader = currentLoader->GetLoader("ADLoader");
    loader->LoadSDigits("READ");
  
    // Get the tree of summable digits
    TTree* sdigitsTree = loader->TreeS();
    if (!sdigitsTree)  {
      AliError("No sdigit tree from digInput");
      continue;
    }

    // Get the branch 
    TBranch* sdigitsBranch = sdigitsTree->GetBranch("ADSDigit");
    if (!sdigitsBranch) {
      AliError("Failed to get sdigit branch");
      return;
    }

    // Set the branch address
    TClonesArray *sdigitsArray = NULL;
    sdigitsBranch->SetAddress(&sdigitsArray);

    // Sum contributions from the sdigits
    // Get number of entries in the tree 
    Int_t nentries  = Int_t(sdigitsBranch->GetEntries());
    for (Int_t entry = 0; entry < nentries; ++entry)  {
      sdigitsBranch->GetEntry(entry);
      // Get the number of sdigits 
      Int_t nsdigits = sdigitsArray->GetEntries();
      
      for (Int_t sdigit = 0; sdigit < nsdigits; sdigit++) {
	AliADSDigit* sDigit = static_cast<AliADSDigit*>(sdigitsArray->UncheckedAt(sdigit));
	Int_t pmNumber = sDigit->PMNumber();
	Int_t nbins = sDigit->GetNBins();
	if (nbins != fNBins[pmNumber]) {
	  AliError(Form("Incompatible number of bins between digitizer (%d) and sdigit (%d) for PM %d! Skipping sdigit!",
			fNBins[pmNumber],nbins,pmNumber));
	  continue;
	}
	// Sum the charges
	Float_t *charges = sDigit->GetCharges();
	for(Int_t iBin = 0; iBin < nbins; ++iBin) fTime[pmNumber][iBin] += charges[iBin];
	// and the labels
	Int_t *labels = sDigit->GetTracks();
	Int_t j = 0;
	for(Int_t i = 0; i < 3; ++i) {
	  if (fLabels[pmNumber][i] < 0) {
	    if (labels[j] < 0) break;
	    fLabels[pmNumber][i] = labels[j];
	    j++;
	  }
	}
      }
    }
    loader->UnloadSDigits();
  }
}


//____________________________________________________________________________
void AliADDigitizer::WriteDigits(AliLoader *loader)
{
  // Take fAdc arrays filled by the previous
  // method and produce and add digits to the digit Tree

  loader->LoadDigits("UPDATE");

  if (!loader->TreeD()) loader->MakeTree("D");
  loader->MakeDigitsContainer();
  TTree* treeD  = loader->TreeD();
  DigitsArray();
  treeD->Branch("ADDigit", &fDigits); 
  
  Short_t *chargeADC = new Short_t[kNClocks];
  for (Int_t i=0; i<16; i++) {      
    for (Int_t j = 0; j < kNClocks; ++j) {
      Int_t tempadc = Int_t(fAdc[i][j]);
      if (tempadc > 1023) tempadc = 1023;
      chargeADC[j] = tempadc;
    }
    AddDigit(i, fLeadingTime[i], fTimeWidth[i], Bool_t((10+fEvenOrOdd)%2), chargeADC, fBBFlag[i], fBGFlag[i], fLabels[i]);
  }
  delete [] chargeADC;

  treeD->Fill();
  loader->WriteDigits("OVERWRITE");  
  loader->UnloadDigits();     
  ResetDigits();
}

//____________________________________________________________________________
void AliADDigitizer::WriteSDigits(AliLoader *loader)
{
  // Take fTime arrays filled by the previous
  // method and produce and add sdigits to the sdigit Tree

  loader->LoadSDigits("UPDATE");

  if (!loader->TreeS()) loader->MakeTree("S");
  loader->MakeSDigitsContainer();
  TTree* treeS  = loader->TreeS();
  SDigitsArray();
  treeS->Branch("ADSDigit", &fDigits); 
  //fAD->MakeBranchInTree(treeS,"AD",&fDigits,8000,"");
  
  for (Int_t ipmt = 0; ipmt < 16; ++ipmt) {
    AddSDigit(ipmt,fNBins[ipmt],fTime[ipmt],fLabels[ipmt]);
  }

  treeS->Fill();
  loader->WriteSDigits("OVERWRITE");  
  loader->UnloadSDigits();     
  ResetDigits();
}



//____________________________________________________________________________
void AliADDigitizer::AddDigit(Int_t pmnumber, Float_t time, Float_t width, Bool_t integrator, Short_t *chargeADC, Bool_t bbFlag, Bool_t bgFlag, Int_t *labels) 
 { 
 
// Adds Digit 
 
  TClonesArray &ldigits = *fDigits;  
	 
  new(ldigits[fNdigits++]) AliADdigit(pmnumber,time,width,integrator,chargeADC,bbFlag,bgFlag,labels);
	 
}
//____________________________________________________________________________
void AliADDigitizer::AddSDigit(Int_t pmnumber, Int_t nbins, Float_t *charges, Int_t *labels) 
 { 
 
// Adds SDigit 
 
  TClonesArray &ldigits = *fDigits;  
	 
  new(ldigits[fNdigits++]) AliADSDigit(pmnumber,nbins,charges,labels);
	 
}
//____________________________________________________________________________
void AliADDigitizer::ResetDigits()
{

// Clears Digits

  fNdigits = 0;
  if (fDigits) fDigits->Clear();
}

//____________________________________________________________________________
AliADCalibData* AliADDigitizer::GetCalibData() const

{
AliCDBManager *man = AliCDBManager::Instance();

  AliCDBEntry *entry=0;

  entry = man->Get("AD/Calib/Data");
  if(!entry){
    AliWarning("Load of calibration data from default storage failed!");
    AliWarning("Calibration data will be loaded from local storage ($ALICE_ROOT)");
	
    man->SetDefaultStorage("local://$ALICE_ROOT/OCDB");
    man->SetRun(1);
    entry = man->Get("AD/Calib/Data");
  }
  // Retrieval of data in directory AD/Calib/Data:

  AliADCalibData *calibdata = 0;

  if (entry) calibdata = (AliADCalibData*) entry->GetObject();
  if (!calibdata)  AliFatal("No calibration data from calibration database !");

  //calibdata->PrintConfig();
  return calibdata;

}

//____________________________________________________________________________
double AliADDigitizer::ChargeSignalShape(double *x, double *par)
{
  // this function simulates the charge shape

  Double_t xx = x[0];
 
  return TMath::Exp(-0.5*TMath::Power(TMath::Log((xx+par[0])/par[1])/par[2],2));
}

//____________________________________________________________________________
double AliADDigitizer::TimeSignalShape(double *x, double *par)
{
  // this function simulates the time shape

  Double_t xx = x[0];
  if (xx <= par[0]) return 0;
  Double_t a = 1./TMath::Power((xx-par[0])/par[1],1./par[2]);
  if (xx <= par[3]) return a;
  Double_t b = 1./TMath::Power((xx-par[3])/par[4],1./par[5]);
  Double_t f = a*b/(a+b);
  AliDebug(100,Form("x=%f func=%f",xx,f));
  return f;
}
//____________________________________________________________________
TClonesArray* AliADDigitizer::DigitsArray() 
{
  // Initialize digit array if not already and
  // return pointer to it. 
  if (!fDigits) { 
    fDigits = new TClonesArray("AliADdigit", 16);
    fNdigits = 0;
  }
  return fDigits;
}

//____________________________________________________________________
TClonesArray* AliADDigitizer::SDigitsArray() 
{
  // Initialize sdigit array if not already and
  // return pointer to it. 
  if (!fDigits) { 
    fDigits = new TClonesArray("AliADSDigit", 16);
    fNdigits = 0;
  }
  return fDigits;
}
