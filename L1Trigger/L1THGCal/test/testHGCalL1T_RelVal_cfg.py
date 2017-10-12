import FWCore.ParameterSet.Config as cms 
from Configuration.StandardSequences.Eras import eras



process = cms.Process('DIGI',eras.Phase2C2)

# import of standard configurations
process.load('Configuration.StandardSequences.Services_cff')
process.load('SimGeneral.HepPDTESSource.pythiapdt_cfi')
process.load('FWCore.MessageService.MessageLogger_cfi')
process.load('Configuration.EventContent.EventContent_cff')
process.load('SimGeneral.MixingModule.mixNoPU_cfi')
process.load('Configuration.Geometry.GeometryExtended2023D17Reco_cff')
process.load('Configuration.Geometry.GeometryExtended2023D17_cff')
process.load('Configuration.StandardSequences.MagneticField_38T_PostLS1_cff')
process.load('Configuration.StandardSequences.Generator_cff')
process.load('IOMC.EventVertexGenerators.VtxSmearedHLLHC14TeV_cfi')
process.load('GeneratorInterface.Core.genFilterSummary_cff')
process.load('Configuration.StandardSequences.SimIdeal_cff')
process.load('Configuration.StandardSequences.Digi_cff')
process.load('Configuration.StandardSequences.SimL1Emulator_cff')
process.load('Configuration.StandardSequences.DigiToRaw_cff')
process.load('Configuration.StandardSequences.EndOfProcess_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')


process.maxEvents = cms.untracked.PSet(
    input = cms.untracked.int32(5)
)

# Input source
process.source = cms.Source("PoolSource",
       fileNames = cms.untracked.vstring('/store/relval/CMSSW_9_1_1_patch1/RelValZEE_14/GEN-SIM-DIGI-RAW/91X_upgrade2023_realistic_v1_D17-v1/10000/38679F3C-B04A-E711-9C7F-0CC47A4D7636.root') )

process.options = cms.untracked.PSet(

)

# Production Info
process.configurationMetadata = cms.untracked.PSet(
    version = cms.untracked.string('$Revision: 1.20 $'),
    annotation = cms.untracked.string('SingleElectronPt10_cfi nevts:10'),
    name = cms.untracked.string('Applications')
)

# Output definition
process.TFileService = cms.Service(
    "TFileService",
    fileName = cms.string("ntuple.root")
    )

# Other statements
from Configuration.AlCa.GlobalTag import GlobalTag
process.GlobalTag = GlobalTag(process.GlobalTag, 'auto:phase2_realistic', '')

# load HGCAL TPG simulation
process.load('L1Trigger.L1THGCal.hgcalTriggerPrimitives_cff')

process.hgcalTriggerPrimitiveDigiProducer.BEConfiguration.algorithms[0].C2d_parameters.seeding_threshold_silicon = cms.double(5)
process.hgcalTriggerPrimitiveDigiProducer.BEConfiguration.algorithms[0].C2d_parameters.clustering_threshold_silicon = cms.double(2)
process.hgcalTriggerPrimitiveDigiProducer.BEConfiguration.algorithms[0].C2d_parameters.seeding_threshold_scintillator = cms.double(5)
process.hgcalTriggerPrimitiveDigiProducer.BEConfiguration.algorithms[0].C2d_parameters.clustering_threshold_scintillator = cms.double(2)
process.hgcalTriggerPrimitiveDigiProducer.BEConfiguration.algorithms[0].C3d_parameters.calibMatrix = cms.bool(True)

import L1Trigger.L1THGCal.hgcalLayersCalibrationCoefficients_cfi as matrixcalibcoeff
process.hgcalTriggerPrimitiveDigiProducer.BEConfiguration.algorithms[0].C3d_parameters.calibCoeffMtx = cms.vdouble( matrixcalibcoeff.dEdx_weights_hadron_wBH )

# load HGCAL TPG simulation
process.load('L1Trigger.L1THGCal.hgcalTriggerPrimitives_cff')
process.hgcl1tpg_step = cms.Path(process.hgcalTriggerPrimitives)
# Change to V7 trigger geometry for older samples
#  from L1Trigger.L1THGCal.customTriggerGeometry import custom_geometry_ZoltanSplit_V7
#  process = custom_geometry_ZoltanSplit_V7(process)

# load ntuplizer
process.load('L1Trigger.L1THGCal.hgcalTriggerNtuples_cff')
process.ntuple_step = cms.Path(process.hgcalTriggerNtuples)

# Schedule definition
process.schedule = cms.Schedule(process.hgcl1tpg_step, process.ntuple_step)

# Add early deletion of temporary data products to reduce peak memory need
from Configuration.StandardSequences.earlyDeleteSettings_cff import customiseEarlyDelete
process = customiseEarlyDelete(process)
# End adding early deletion

