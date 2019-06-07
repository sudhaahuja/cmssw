#include "SimCalorimetry/HGCalSimProducers/interface/HGCDigitizerBase.h"
#include "Geometry/HGCalGeometry/interface/HGCalGeometry.h"
#include "Geometry/HcalTowerAlgo/interface/HcalGeometry.h"

using namespace hgc_digi;
using namespace hgc_digi_utils;

template<class DFr>
HGCDigitizerBase<DFr>::HGCDigitizerBase(const edm::ParameterSet& ps) : scaleByDose_(false) {
  bxTime_        = ps.getParameter<double>("bxTime");
  myCfg_         = ps.getParameter<edm::ParameterSet>("digiCfg");
  doTimeSamples_ = myCfg_.getParameter< bool >("doTimeSamples");
  
  if(myCfg_.exists("keV2fC"))   keV2fC_   = myCfg_.getParameter<double>("keV2fC");
  else                          keV2fC_   = 1.0;

  if( myCfg_.existsAs<edm::ParameterSet>( "chargeCollectionEfficiencies" ) ) {
    cce_ = myCfg_.getParameter<edm::ParameterSet>("chargeCollectionEfficiencies").template getParameter<std::vector<double>>("values");
  }

  if(myCfg_.existsAs<double>("noise_fC")) {
    noise_fC_.reserve(1);
    noise_fC_.push_back(myCfg_.getParameter<double>("noise_fC"));
  } else if ( myCfg_.existsAs<std::vector<double> >("noise_fC") ) {
    const auto& noises = myCfg_.getParameter<std::vector<double> >("noise_fC");
    noise_fC_ = std::vector<float>(noises.begin(),noises.end());
  } else if(myCfg_.existsAs<edm::ParameterSet>("noise_fC")) {
    const auto& noises = myCfg_.getParameter<edm::ParameterSet>("noise_fC").template getParameter<std::vector<double> >("values");
    noise_fC_ = std::vector<float>(noises.begin(),noises.end());
    scaleByDose_ = myCfg_.getParameter<edm::ParameterSet>("noise_fC").template getParameter<bool>("scaleByDose");
    doseMapFile_ = myCfg_.getParameter<edm::ParameterSet>("noise_fC").template getParameter<std::string>("doseMap");
    scal_.setDoseMap(doseMapFile_);
  } else {
    noise_fC_.resize(1,1.f);
  }
  edm::ParameterSet feCfg = myCfg_.getParameter<edm::ParameterSet>("feCfg");
  myFEelectronics_        = std::unique_ptr<HGCFEElectronics<DFr> >( new HGCFEElectronics<DFr>(feCfg) );
  myFEelectronics_->SetNoiseValues(noise_fC_);
}

template<class DFr>
void HGCDigitizerBase<DFr>::run( std::unique_ptr<HGCDigitizerBase::DColl> &digiColl,
				 HGCSimHitDataAccumulator &simData,
				 const CaloSubdetectorGeometry* theGeom,
				 const std::unordered_set<DetId>& validIds,
				 uint32_t digitizationType,
				 CLHEP::HepRandomEngine* engine) {
  if(scaleByDose_)  scal_.setGeometry(theGeom);
  if(digitizationType==0) runSimple(digiColl,simData,theGeom,validIds,engine);
  else                    runDigitizer(digiColl,simData,theGeom,validIds,digitizationType,engine);
}

template<class DFr>
void HGCDigitizerBase<DFr>::runSimple(std::unique_ptr<HGCDigitizerBase::DColl> &coll,
				      HGCSimHitDataAccumulator &simData,
				      const CaloSubdetectorGeometry* theGeom,
				      const std::unordered_set<DetId>& validIds,
				      CLHEP::HepRandomEngine* engine) {
  HGCSimHitData chargeColl,toa;

  // this represents a cell with no signal charge
  HGCCellInfo zeroData;
  zeroData.hit_info[0].fill(0.f); //accumulated energy
  zeroData.hit_info[1].fill(0.f); //time-of-flight

  for( const auto& id : validIds ) {
    chargeColl.fill(0.f);
    toa.fill(0.f);
    HGCSimHitDataAccumulator::iterator it = simData.find(id);
    HGCCellInfo& cell = ( simData.end() == it ? zeroData : it->second );
    addCellMetadata(cell,theGeom,id);

    float cce(1.f),noise(0.f);
    for(size_t i=0; i<cell.hit_info[0].size(); i++) {
      double rawCharge(cell.hit_info[0][i]);

      //time of arrival
      toa[i]=cell.hit_info[1][i];
      if(myFEelectronics_->toaMode()==HGCFEElectronics<DFr>::WEIGHTEDBYE && rawCharge>0)
        toa[i]=cell.hit_info[1][i]/rawCharge;

      //convert total energy in GeV to charge (fC)
      //double totalEn=rawEn*1e6*keV2fC_;
      float totalCharge=rawCharge;

      //add noise (in fC)
      //we assume it's randomly distributed and won't impact ToA measurement
      //also assume that it is related to the charge path only and that noise fluctuation for ToA circuit be handled separately

      if(scaleByDose_){

        //@discuss should this be done after cce? maybe..., on the other hand it's the Ileak term that should dominate...
        HGCalSiNoiseMap::SignalRange_t sigRange(HGCalSiNoiseMap::q80fC);
        if(totalCharge>80){
          sigRange=HGCalSiNoiseMap::q160fC;
          if(totalCharge>320){
            sigRange=HGCalSiNoiseMap::q320fC;
          }
        }

        HGCSiliconDetId detId(id);

        //the cce depends only on the DetId but the noise may change depending on the charge deposited
        HGCalSiNoiseMap::SiCellOpCharacteristics siop=scal_.getSiCellOpCharacteristics(sigRange,detId);
        cce   = siop.cce;
        noise = std::max( (float)CLHEP::RandGaussQ::shoot(engine,0.0,siop.noise),0.f);
      }
      else if (noise_fC_[cell.thickness-1] != 0) {
        cce   = (cce_.empty() ? 1.f : cce_[cell.thickness-1]);
        noise = std::max( (float)CLHEP::RandGaussQ::shoot(engine,0.0,cell.size*noise_fC_[cell.thickness-1]) , 0.f );
      }

      //@discuss calibrated (S.cce+N) \approx S+N/cce, prefer to keep digi calibration here 
      totalCharge+=noise/cce;
      if(totalCharge<0.f) totalCharge=0.f;

      chargeColl[i]=totalCharge;
    }

    //run the shaper to create a new data frame
    DFr rawDataFrame( id );
    myFEelectronics_->runShaper(rawDataFrame, chargeColl, toa, cell.thickness, engine, cce);

    //update the output according to the final shape
    updateOutput(coll,rawDataFrame);
  }
}

template<class DFr>
void HGCDigitizerBase<DFr>::updateOutput(std::unique_ptr<HGCDigitizerBase::DColl> &coll,
                                          const DFr& rawDataFrame) {
  int itIdx(9);
  if(rawDataFrame.size()<=itIdx+2) return;

  DFr dataFrame( rawDataFrame.id() );
  dataFrame.resize(5);
  bool putInEvent(false);
  for(int it=0;it<5; it++) {
    dataFrame.setSample(it, rawDataFrame[itIdx-2+it]);
    if(it==2) putInEvent = rawDataFrame[itIdx-2+it].threshold();
  }

  if(putInEvent) {
    coll->push_back(dataFrame);
  }
}

// cause the compiler to generate the appropriate code
#include "DataFormats/HGCDigi/interface/HGCDigiCollections.h"
template class HGCDigitizerBase<HGCEEDataFrame>;
template class HGCDigitizerBase<HGCBHDataFrame>;
template class HGCDigitizerBase<HGCalDataFrame>;
