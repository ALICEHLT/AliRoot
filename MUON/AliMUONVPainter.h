#ifndef ALIMUONVPAINTER_H
#define ALIMUONVPAINTER_H

/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
* See cxx source for full Copyright notice                               */

// $Id$

/// \ingroup graphics
/// \class AliMUONVPainter
/// \brief Base class for a graphical object representing some part of the
/// MUON tracking system
/// 
// Author Laurent Aphecetche, Subatech

#ifndef ALIMUONATTPAINTER_H
#  include "AliMUONAttPainter.h"
#endif
#ifndef ROOT_RQ_OBJECT
#  include <RQ_OBJECT.h>
#endif
#ifndef ROOT_TString
#  include "TString.h"
#endif
#ifndef ROOT_TObject
#  include "TObject.h"
#endif
#ifndef AL_MP_AREA_H
#  include "AliMpArea.h"
#endif
#include <float.h>

class AliMUONPainterContour;
class AliMUONPainterGroup;
class AliMUONVTrackerData;
class AliMpArea;
class TCollection;
class TList;
class TMap;
class TObjArray;
class TVirtualPad;

class AliMUONVPainter : public TObject
{
  RQ_OBJECT("AliMUONVPainter")

public:  

  AliMUONVPainter(const char* type="");
  AliMUONVPainter(const AliMUONVPainter& rhs);
  AliMUONVPainter& operator=(const AliMUONVPainter& rhs);
  virtual ~AliMUONVPainter();

  /// Add a painter to our list of children. We adopt this painter (i.e. we become owner).
  void Add(AliMUONVPainter* painter);
  
  /// Return the area containing this painter
  AliMpArea Area() const;
  
  virtual void SetAttributes(const AliMUONAttPainter& attributes);
  
  /// Convert attributes so they are valid ones for us.
  virtual AliMUONAttPainter Validate(const AliMUONAttPainter& attributes) const { return attributes; }
  
  const AliMUONAttPainter& Attributes() const { return fAttributes; }
  
  virtual void ComputeDataRange(const AliMUONVTrackerData& data, Int_t dataIndex, 
                                Double_t& dataMin, Double_t& dataMax) const;
  
  virtual void Copy(TObject& object) const;
  
  AliMUONVPainter* Detach() const;
  
  virtual Bool_t IsValid() const { return fIsValid; }
  
  void Invalidate() { fIsValid = kFALSE; }
  
  Int_t Depth() const;
  
  virtual Int_t	DistancetoPrimitive(Int_t px, Int_t py);
  
  virtual void Draw(Option_t* opt="");
  
  virtual void ExecuteEvent(Int_t event, Int_t px, Int_t py);
  
  /// Return the contour representing the outline of this object
  AliMUONPainterContour* Contour() const { return fContour; }

  virtual const char* GetName() const { return Name().Data(); }
  
  virtual TString Name() const { return fName; }
  
  virtual TString PathName() const { return fPathName; }
  
  virtual TString ContourName() const;

  virtual char*	GetObjectInfo(Int_t px, Int_t py) const;
  
  void GetTypes(TObjArray& types) const;

  /// Return our mother group
  AliMUONPainterGroup* MotherGroup() const { return fGroup; }
  
  /// Return specific name at a given position, if needed.
  virtual TString NameAtPosition(Double_t /*x*/, Double_t /*y*/) const { return GetName(); }
  
  AliMUONPainterGroup* Group(const char* type) const;
  
  AliMUONPainterGroup* Group(Int_t depth) const;
  
  virtual Bool_t HandleMouseMotion() const { return kFALSE; }
  
  Bool_t IsResponder() const;
  
  Bool_t IsInside(Double_t x, Double_t y) const;

  /// Return our mother (0 if we're the top node)
  AliMUONVPainter* Mother() const { return fMother; }

  virtual void Paint(Option_t* opt="");

  virtual void Print(Option_t* opt="") const;

  /// Return the plotter group
  AliMUONPainterGroup* PlotterGroup() const { return fPlotterGroup; }
  
  /// Return the responder group
  AliMUONPainterGroup* ResponderGroup() const { return fResponderGroup; }

  /// Set out contour
  void SetContour(AliMUONPainterContour* contour);
  
  void SetData(const char* pattern, AliMUONVTrackerData* data, Int_t dataIndex);

  void SetLine(Int_t depth, Int_t lineColor, Int_t lineWidth);
  
  /// Set our mother group
  void SetMotherGroup(AliMUONPainterGroup* group) { fGroup = group; }
  
  void SetMother(AliMUONVPainter* painter);
  
  void SetOutlined(const char* pattern, Bool_t flag);
  
  void SetResponder(const char* pattern);
  
  void SetResponder(Int_t depth);
  
  void SetVisible(const char* pattern, Bool_t flag);
  
  /// Return our type (e.g. PCB, Chamber, DE, MANU, etc...)
  const char* Type() const { return fType.Data(); }

  // SIGNALS
  
  void Clicked(AliMUONVPainter* painter, Double_t* pos); // *SIGNAL*

  void ShiftClicked(AliMUONVPainter* painter, Double_t* pos); // *SIGNAL*

  void DoubleClicked(AliMUONVPainter* painter, Double_t* pos); // *SIGNAL*
    
  static void PixelToPad(Int_t px, Int_t py, Double_t& x, Double_t& y);

  virtual void PaintOutline(Int_t color=-1, Int_t width=-1, Double_t x=FLT_MAX, Double_t y=FLT_MAX);

  virtual void PaintArea(const AliMUONVTrackerData& data, Int_t dataIndex,
                         Double_t min, Double_t max);
    
  TVirtualPad* Pad() const { return fPad; }
  
  Int_t GetLineColor() const { return fLineColor; }
  
  Int_t GetLineWidth() const { return fLineWidth; }
  
  void SetLineColor(Int_t lineColor) { fLineColor = lineColor; }
  
  void SetLineWidth(Int_t lineWidth) { fLineWidth = lineWidth; }
  
  void SetName(const char* name) { fName = name; }
  
  void SetPathName(const char* pathName) { fPathName = pathName; }
  
  static AliMUONVPainter* CreatePainter(const char* className, 
                                        const AliMUONAttPainter& att,
                                        Int_t id1, Int_t id2);
    
  Int_t ID0() const { return fID[0]; }
  Int_t ID1() const { return fID[1]; }
  
  void SetID(Int_t id0, Int_t id1) { fID[0] = id0; fID[1] = id1; }
  
  virtual TString Describe(const AliMUONVTrackerData& data, Int_t dataIndex,
                           Double_t x=FLT_MAX, Double_t y=FLT_MAX);

  void UpdateGroupsFrom(const AliMUONVPainter& painter);

  AliMUONVPainter* Master() const;
  
protected:
    
  virtual TCollection* Children() const;
  
private:

    void FlatList(TList& list);

  AliMUONPainterGroup* CreateGroup(const char* type, Int_t depth);
  
  void CreateGroups();
  
  void GetBoundingBox(Double_t& x1, Double_t& y1, 
                      Double_t& x2, Double_t& y2) const;
  
  AliMUONVPainter* GetPainter(Int_t px, Int_t py, Double_t& x, Double_t& y) const;
  
private:
  
  TString fName; ///< our (short) name
  TString fPathName; ///< our long name
  TString fType; ///< our type (DE, Chamber, MANU, etc...)
  AliMUONVPainter* fMother;  ///< our mother
  AliMUONPainterGroup* fGroup; ///< our group
  AliMUONPainterContour* fContour;  ///< our contour
  TMap* fPainterGroups; ///< map of groups
  TObjArray* fChildren; ///< our children
  AliMUONPainterGroup* fResponderGroup; ///< the responder group
  AliMUONPainterGroup* fPlotterGroup; ///< the plotter group
  Double_t fBorderFactor; ///< border factor for visu 
  TVirtualPad* fPad; ///< the pad we're drawn in
  AliMUONAttPainter fAttributes; ///< our attributes (e.g. view type)
  Int_t fLineColor; ///< our outline color
  Int_t fLineWidth; ///< our outline width
  Int_t fID[2]; ///< our ids
  Bool_t fIsValid; ///< whether we were properly initialized
  
  ClassDef(AliMUONVPainter,1) // Base class of a graphical object for MUON spectrometer
};

#endif
