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

//-------------------------------------------------------------------------
//               Implementation of the cascade vertex class
//
//    Origin: Christian Kuhn, IReS, Strasbourg, christian.kuhn@ires.in2p3.fr
//-------------------------------------------------------------------------
#include <iostream.h>
#include <TMath.h>

#include "AliCascadeVertex.h"
#include "AliV0vertex.h"
#include "AliITStrackV2.h"

ClassImp(AliCascadeVertex)

AliCascadeVertex::AliCascadeVertex() : TObject() {
  //--------------------------------------------------------------------
  // Default constructor  (Xi-)
  //--------------------------------------------------------------------
  fPdgCode=kXiMinus;
  fEffMass=1.32131;
  fChi2=1.e+33;
  fPos[0]=fPos[1]=fPos[2]=0.;
  fPosCov[0]=fPosCov[1]=fPosCov[2]=fPosCov[3]=fPosCov[4]=fPosCov[5]=0.;
}



inline Double_t det(Double_t a00, Double_t a01, Double_t a10, Double_t a11){
  // determinant 2x2
  return a00*a11 - a01*a10;
}

inline Double_t det (Double_t a00,Double_t a01,Double_t a02,
                     Double_t a10,Double_t a11,Double_t a12,
                     Double_t a20,Double_t a21,Double_t a22) {
  // determinant 3x3
  return 
  a00*det(a11,a12,a21,a22)-a01*det(a10,a12,a20,a22)+a02*det(a10,a11,a20,a21);
}



AliCascadeVertex::AliCascadeVertex(const AliV0vertex &v,const AliITStrackV2 &t) {
  //--------------------------------------------------------------------
  // Main constructor
  //--------------------------------------------------------------------
  fPdgCode=kXiMinus;

  fV0lab[0]=v.GetNlabel(); fV0lab[1]=v.GetPlabel();
  fBachLab=t.GetLabel(); 

  //Trivial estimation of the vertex parameters
  Double_t pt, phi, x, par[5];
  Double_t alpha, cs, sn;

  t.GetExternalParameters(x,par); alpha=t.GetAlpha();
  pt=1./TMath::Abs(par[4]);
  phi=TMath::ASin(par[2]) + alpha;  

  // momentum of the bachelor track

  Double_t px1=pt*TMath::Cos(phi), py1=pt*TMath::Sin(phi), pz1=pt*par[3];

  cs=TMath::Cos(alpha); sn=TMath::Sin(alpha);

  Double_t x1=x*cs - par[0]*sn; // position of the bachelor at dca (bachelor,V0)
  Double_t y1=x*sn + par[0]*cs;
  Double_t z1=par[1];

  Double_t x2,y2,z2;          // position of the V0 
  v.GetXYZ(x2,y2,z2);
    
  Double_t px2,py2,pz2;       // momentum of V0
  v.GetPxPyPz(px2,py2,pz2);

  Double_t a2=((x1-x2)*px2+(y1-y2)*py2+(z1-z2)*pz2)/(px2*px2+py2*py2+pz2*pz2);

  Double_t xm=x2+a2*px2;
  Double_t ym=y2+a2*py2;
  Double_t zm=z2+a2*pz2;

  // position of the cascade decay
  
  fPos[0]=0.5*(x1+xm); fPos[1]=0.5*(y1+ym); fPos[2]=0.5*(z1+zm);
  
  
  // momenta of the bachelor and the V0
  
  fBachMom[0]=px1; fBachMom[1]=py1; fBachMom[2]=pz1; 
  fV0mom[0]=px2; fV0mom[1]=py2; fV0mom[2]=pz2;
  
  // invariant mass of the cascade (default is Ximinus)
  
  Double_t e1=TMath::Sqrt(0.13957*0.13957 + px1*px1 + py1*py1 + pz1*pz1);
  Double_t e2=TMath::Sqrt(1.11568*1.11568 + px2*px2 + py2*py2 + pz2*pz2);
  
  fEffMass=TMath::Sqrt((e1+e2)*(e1+e2)-
    (px1+px2)*(px1+px2)-(py1+py2)*(py1+py2)-(pz1+pz2)*(pz1+pz2));

  fChi2=7.;   

}

void AliCascadeVertex::ChangeMassHypothesis(Int_t code) {
  //--------------------------------------------------------------------
  // This function changes the mass hypothesis for this cascade
  //--------------------------------------------------------------------
  
  // HOW TO DISTINGUISH BETWEEN A XIMINUS AND A XIPLUS ??????????
  // SAME QUESTION FOR (ANTI-)OMEGA'S (here) ... AND FOR (ANTI-)LAMBDAS (in AliV0vertex) ??
  // -> NEED ADDITIONAL CONDITION ON BACHELOR AND V0 PDGCODE !!!! BUT in the ANALYSIS MACROS !!!
  
  Double_t massBach, massV0;

  switch (code) {

  case kXiMinus:
    massBach=0.13957; massV0=1.11568; break;
  case kXiPlusBar:
    massBach=0.13957; massV0=1.11568; break;
  case kOmegaMinus: 
    massBach=0.49368; massV0=1.11568; break;
  case kOmegaPlusBar: 
    massBach=0.49368; massV0=1.11568; break;

  default:
    cerr<<"AliCascadeVertex::ChangeMassHypothesis: ";
    cerr<<"invalide PDG code ! Assuming XiMinus's...\n";
    massBach=0.13957; massV0=1.11568; break;
  }

  Double_t px1=fBachMom[0], py1=fBachMom[1], pz1=fBachMom[2]; 
  Double_t px2=fV0mom[0], py2=fV0mom[1], pz2=fV0mom[2];

  Double_t e1=TMath::Sqrt(massBach*massBach + px1*px1 + py1*py1 + pz1*pz1);
  Double_t e2=TMath::Sqrt(massV0*massV0 + px2*px2 + py2*py2 + pz2*pz2);
  fEffMass=TMath::Sqrt((e1+e2)*(e1+e2)-
    (px1+px2)*(px1+px2)-(py1+py2)*(py1+py2)-(pz1+pz2)*(pz1+pz2));
  
  fPdgCode=code;
}

void 
AliCascadeVertex::GetPxPyPz(Double_t &px, Double_t &py, Double_t &pz) const {
  //--------------------------------------------------------------------
  // This function returns the cascade momentum (global)
  //--------------------------------------------------------------------
  px=fV0mom[0]+fBachMom[0]; 
  py=fV0mom[1]+fBachMom[1]; 
  pz=fV0mom[2]+fBachMom[2]; 
}

void AliCascadeVertex::GetXYZ(Double_t &x, Double_t &y, Double_t &z) const {
  //--------------------------------------------------------------------
  // This function returns cascade position (global)
  //--------------------------------------------------------------------
  x=fPos[0]; 
  y=fPos[1]; 
  z=fPos[2]; 
}

Double_t AliCascadeVertex::GetD(Double_t x0, Double_t y0, Double_t z0) const {
  //--------------------------------------------------------------------
  // This function returns the cascade impact parameter
  //--------------------------------------------------------------------

  Double_t x=fPos[0],y=fPos[1],z=fPos[2];
  Double_t px=fV0mom[0]+fBachMom[0];
  Double_t py=fV0mom[1]+fBachMom[1];
  Double_t pz=fV0mom[2]+fBachMom[2];

  Double_t dx=(y0-y)*pz - (z0-z)*py; 
  Double_t dy=(x0-x)*pz - (z0-z)*px;
  Double_t dz=(x0-x)*py - (y0-y)*px;
  Double_t d=TMath::Sqrt((dx*dx+dy*dy+dz*dz)/(px*px+py*py+pz*pz));

  return d;
}
