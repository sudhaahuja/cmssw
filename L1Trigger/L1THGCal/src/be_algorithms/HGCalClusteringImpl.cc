#include "L1Trigger/L1THGCal/interface/be_algorithms/HGCalClusteringImpl.h"
#include "DataFormats/Common/interface/PtrVector.h"
#include "DataFormats/Common/interface/OrphanHandle.h"

//class constructor
HGCalClusteringImpl::HGCalClusteringImpl(const edm::ParameterSet & conf):
    seedThreshold_(conf.getParameter<double>("seeding_threshold")),
    triggerCellThreshold_(conf.getParameter<double>("clustering_threshold")),
    dr_(conf.getParameter<double>("dR_cluster"))
{    

    edm::LogInfo("HGCalClusterParameters") << "C2d seeding Thr: " << seedThreshold_ ; 
    edm::LogInfo("HGCalClusterParameters") << "C2d clustering Thr: " << triggerCellThreshold_ ; 
 
}


bool HGCalClusteringImpl::isPertinent( const l1t::HGCalTriggerCell & tc, 
                                       const l1t::HGCalCluster & clu, 
                                       double distXY ) const 
{

    HGCalDetId tcDetId( tc.detId() );
    HGCalDetId cluDetId( clu.seedDetId() );
    if( (tcDetId.layer() != cluDetId.layer()) ||
        (tcDetId.subdetId() != cluDetId.subdetId()) ||
        (tcDetId.zside() != cluDetId.zside()) ){
        return false;
    }   
    if ( clu.distance((tc)) < distXY ){
        return true;
    }
    return false;

}


void HGCalClusteringImpl::clusterize( const edm::PtrVector<l1t::HGCalTriggerCell> & triggerCellsPtrs, 
                                      l1t::HGCalClusterBxCollection & clusters
    ){
    
    bool isSeed[triggerCellsPtrs.size()];
    
    /* search for cluster seeds */
    int itc=0;
    for( edm::PtrVector<l1t::HGCalTriggerCell>::const_iterator tc = triggerCellsPtrs.begin(); tc != triggerCellsPtrs.end(); ++tc,++itc ){
        isSeed[itc] = ( (*tc)->mipPt() > seedThreshold_) ? true : false;
    }
    
    /* clustering the TCs */
    std::vector<l1t::HGCalCluster> clustersTmp;

    itc=0;
    for( edm::PtrVector<l1t::HGCalTriggerCell>::const_iterator tc = triggerCellsPtrs.begin(); tc != triggerCellsPtrs.end(); ++tc,++itc ){
            
        if( (*tc)->mipPt() < triggerCellThreshold_ ){
            continue;
        }
        
        /* searching for TC near the center of the cluster  */
        int iclu=0;
        vector<int> tcPertinentClusters; 
        for( std::vector<l1t::HGCalCluster>::iterator clu = clustersTmp.begin(); clu != clustersTmp.end(); ++clu,++iclu ){
            if( this->isPertinent(**tc, *clu, dr_) ){
                tcPertinentClusters.push_back(iclu);
            }
        }
        if( tcPertinentClusters.size() == 0 && isSeed[itc] ){
            clustersTmp.emplace_back( *tc );
        }
        else if ( tcPertinentClusters.size() > 0 ){
         
            unsigned minDist = 300;
            unsigned targetClu = 0;
                        
            for( std::vector<int>::const_iterator iclu = tcPertinentClusters.begin(); iclu != tcPertinentClusters.end(); ++iclu ){
                double d = clustersTmp.at(*iclu).distance(**tc);
                if( d < minDist ){
                    minDist = d;
                    targetClu = *iclu;
                }
            } 

            clustersTmp.at(targetClu).addTriggerCell( *tc );                    

        }
    }

    /* making the collection of clusters */
    for( unsigned i(0); i<clustersTmp.size(); ++i ){
        clusters.push_back( 0, clustersTmp.at(i) );
    }
    
}
