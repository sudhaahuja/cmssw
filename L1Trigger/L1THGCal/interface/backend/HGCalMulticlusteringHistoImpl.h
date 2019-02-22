#ifndef __L1Trigger_L1THGCal_HGCalMulticlusteringHistoImpl_h__
#define __L1Trigger_L1THGCal_HGCalMulticlusteringHistoImpl_h__

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "DataFormats/L1THGCal/interface/HGCalCluster.h"
#include "DataFormats/L1THGCal/interface/HGCalMulticluster.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include "L1Trigger/L1THGCal/interface/HGCalTriggerGeometryBase.h"
#include "L1Trigger/L1THGCal/interface/backend/HGCalShowerShape.h"
#include "L1Trigger/L1THGCal/interface/HGCalTriggerTools.h"
#include "L1Trigger/L1THGCal/interface/backend/HGCalTriggerClusterIdentificationBase.h"


class HGCalMulticlusteringHistoImpl{

public:

    HGCalMulticlusteringHistoImpl( const edm::ParameterSet &conf);    

    void eventSetup(const edm::EventSetup& es) 
    {
        triggerTools_.eventSetup(es);
        shape_.eventSetup(es);
    }

    float dR( const l1t::HGCalCluster & clu,
	      const GlobalPoint & seed ) const;

    void clusterizeHisto( const std::vector<edm::Ptr<l1t::HGCalCluster>> & clustersPtr,
			  l1t::HGCalMulticlusterBxCollection & multiclusters,
			  const HGCalTriggerGeometryBase & triggerGeometry
			  );


private:
    enum MulticlusterType{
      HistoMaxC3d,
      HistoSecondaryMaxC3d,
      HistoThresholdC3d,
      HistoInterpolatedMaxC3d
    };

    enum ClusterAssociationStrategy{
      NearestNeighbour,
      EnergySplit
    };

    enum ClusterRadiusStrategy{
      Fixed,
      LinearWithEta
    };

    typedef std::map<std::array<int,3>,float> Histogram;

    Histogram fillHistoClusters( const std::vector<edm::Ptr<l1t::HGCalCluster>> & clustersPtrs );

    Histogram fillSmoothPhiHistoClusters( const Histogram & histoClusters,
					  const vector<unsigned> & binSums );

    Histogram fillSmoothRPhiHistoClusters( const Histogram & histoClusters );

    std::vector<std::pair<GlobalPoint, double> > computeMaxSeeds( const Histogram & histoClusters );

    std::vector<std::pair<GlobalPoint, double> > computeSecondaryMaxSeeds( const Histogram & histoClusters );
    
    std::vector<std::pair<GlobalPoint, double> > computeInterpolatedMaxSeeds( const Histogram & histoClusters );
    
    std::vector<std::pair<GlobalPoint, double> > computeThresholdSeeds( const Histogram & histoClusters );

    std::vector<l1t::HGCalMulticluster> clusterSeedMulticluster(const std::vector<edm::Ptr<l1t::HGCalCluster>> & clustersPtrs,
								const std::vector<GlobalPoint> & seeds);

    std::vector<l1t::HGCalMulticluster> clusterSeedMulticluster(const std::vector<edm::Ptr<l1t::HGCalCluster>> & clustersPtrs,
								const std::vector<std::pair<GlobalPoint, double> > & seeds);


    void finalizeClusters(std::vector<l1t::HGCalMulticluster>&,
            l1t::HGCalMulticlusterBxCollection&,
            const HGCalTriggerGeometryBase&);
    
    double dr_;
    double radiusCoefficientA_;
    double radiusCoefficientB_;
    double ptC3dThreshold_;

    std::string multiclusterAlgoType_;
    std::string cluster_association_input_;
    std::string cluster_radius_input_;
    MulticlusterType multiclusteringAlgoType_;
    ClusterAssociationStrategy cluster_association_strategy_;
    ClusterRadiusStrategy cluster_radius_strategy_;

    unsigned nBinsRHisto_ = 36;
    unsigned nBinsPhiHisto_ = 216;
    std::vector<unsigned> binsSumsHisto_;
    double histoThreshold_ = 20.;
    std::vector<double> neighbour_weights_;

    HGCalShowerShape shape_;
    HGCalTriggerTools triggerTools_;
    std::unique_ptr<HGCalTriggerClusterIdentificationBase> id_;

    static constexpr unsigned neighbour_weights_size_ = 9;
    static constexpr double kROverZMin_ = 0.09;
    static constexpr double kROverZMax_ = 0.52;

    static constexpr double kMidRadius_ = 2.3;

};

#endif
