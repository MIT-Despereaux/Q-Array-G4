/*
This class performs as the MC sampler.
*/

#ifndef QRMCSampler_h
#define QRMCSampler_h 1

#define MCMC_ENABLE_EIGEN_WRAPPERS
#include "mcmc.hpp"
#include <Eigen/Dense>

#include "G4String.hh"
#include "G4SystemOfUnits.hh"
#include "G4Exception.hh"
#include "G4ios.hh"
#include "Randomize.hh"

#include <vector>
#include <map>
#include <math.h>

// Distribution data
namespace DistData
{
    static const std::vector<double> RHO = {0.102573, -0.068287, 0.958633, 0.0407253, 0.817285};
    static const std::vector<double> EPSILON = {115 * GeV, 850 * GeV};
    static const double E0 = 3.64 * GeV;
    static const double GAMMA = 2.7;
};

namespace QR
{

    // MC sampler
    class MCSampler
    {
    public:
        enum TYPE
        {
            AEES = 0,
            RWMH
        };

    private:
        std::map<G4String, std::vector<double>> mRanges;
        mcmc::algo_settings_t mSettings;
        Eigen::VectorXd initialVals;
        // Eigen::MatrixXd covMatrix;
        int data = 0;
        u_int usedDraw = 0;

        static double DefaultFluxPDF(const Eigen::VectorXd &valsInput, void *data)
        {
            const double E = valsInput(0);
            const double theta = valsInput(1);
            // G4cout << E << " " << theta << G4endl;
            double z_1 = pow(cos(theta), 2.) + pow(DistData::RHO[0], 2.) + DistData::RHO[1] * pow(cos(theta), DistData::RHO[2]) + DistData::RHO[3] * pow(cos(theta), DistData::RHO[4]);
            double z_2 = 1. + pow(DistData::RHO[0], 2.) + DistData::RHO[1] + DistData::RHO[3];
            double z = sqrt(z_1 / z_2);
            double q = 1. + DistData::E0 / (E * pow(z, 1.29));
            double pdf = pow((E * q), -DistData::GAMMA) * (1. / (1. + 1.1 * E * z / DistData::EPSILON[0]) + 0.054 / (1. + 1.1 * E * z / DistData::EPSILON[1]));
            return pdf * sin(theta);
        }

        // Return the log likelihood function
        static double LLDensity(const Eigen::VectorXd &valsInput, void *data)
        {
            return log(DefaultFluxPDF(valsInput, data));
        }

        // Draw samples, assuming the sampler is properly initialized
        void DrawSamples(TYPE type = AEES)
        {
            switch (type)
            {
            case AEES:
                mcmc::aees(initialVals, LLDensity, draws, &data, mSettings);
                break;
            case RWMH:
                mcmc::rwmh(initialVals, LLDensity, draws, &data, mSettings);
                break;
            default:
                G4Exception("MCSampler::DrawSamples", "InvalidInput", FatalException, "Invalid sampler type");
                break;
            }

            // G4cout << "Mean of first parameter: " << draws.col(0).mean() << G4endl;
            // G4cout << "Mean of second parameter: " << draws.col(1).mean() << G4endl;

            // Need to update the initial values to the last sample
            G4cout << "N rows: " << draws.rows() << " N cols: " << draws.cols() << G4endl;
            // G4cout << draws({1,2,3,4,5}, Eigen::all) << G4endl;
            initialVals = draws.row(nSamples - 1).transpose();
            G4cout << "Initial values updated to: " << initialVals.transpose() << G4endl;
        }

    public:
        Eigen::MatrixXd draws;
        u_int nSamples = 50000;
        // Tells if the sampler is ready to generate a single sample.
        bool bInitialized = false;

        // Constructor
        MCSampler()
        {
            initialVals = Eigen::VectorXd::Zero(2);
            // covMatrix = Eigen::MatrixXd::Identity(2, 2);
            mSettings.lower_bounds = Eigen::VectorXd::Zero(2);
            mSettings.upper_bounds = Eigen::VectorXd::Zero(2);
            // mSettings.aees_settings.cov_mat = Eigen::MatrixXd::Identity(2, 2);
            SetParameterE(0.5 * GeV, 1000 * GeV);
            SetParameterTheta(0, M_PI / 2);
        }

        // Destructor
        ~MCSampler() {}

        void SetSeed(size_t seed) // new method to set the seed
        {
            mSettings.rng_seed_value = seed;
        }

        // Set the parameter ranges
        void SetParameterE(double minE, double maxE)
        {
            mRanges.insert({"E_mu", {minE, maxE}});
            initialVals(0) = (minE + maxE) / 2;
            // covMatrix(0, 0) = (maxE - minE) / 50;
            mSettings.lower_bounds(0) = minE;
            mSettings.upper_bounds(0) = maxE;
            bInitialized = false;
        }

        void SetParameterTheta(double minTheta, double maxTheta)
        {
            mRanges.insert({"theta", {minTheta, maxTheta}});
            initialVals(1) = (minTheta + maxTheta) / 2;
            // covMatrix(1, 1) = (maxTheta - minTheta) / 100;
            mSettings.lower_bounds(1) = minTheta;
            mSettings.upper_bounds(1) = maxTheta;
            bInitialized = false;
        }

        void Initialize()
        {
            mSettings.vals_bound = true;

            mSettings.aees_settings.n_initial_draws = 2000;
            mSettings.aees_settings.n_burnin_draws = 2000;
            mSettings.rwmh_settings.n_burnin_draws = 2000;

            // Check nSamples >= 1
            if (nSamples < 1)
            {
                G4Exception("MCSampler::DrawSamples", "InvalidInput", FatalException, "nSamples must be >= 1");
            }
            mSettings.aees_settings.n_keep_draws = nSamples;
            mSettings.rwmh_settings.n_keep_draws = nSamples;
            usedDraw = nSamples; // Make sure to draw new samples
            bInitialized = true;
        }

        void DrawOneSample(G4double &theta, G4double &energy, TYPE type = AEES)
        {
            if (!bInitialized)
            {
                G4Exception("MCSampler::DrawOneSample", "InvalidInput", FatalException, "Sampler not initialized");
            }
            if (usedDraw > draws.rows() - 10)
            {
                long seed = G4Random::getTheSeed();
                SetSeed(static_cast<size_t>(seed));
                DrawSamples(type);
                usedDraw = 0;
            }
            theta = draws(usedDraw, 1);
            energy = draws(usedDraw, 0);
            // G4cout << "theta: " << theta << " energy: " << energy << G4endl;
            // G4cout << "usedDraw: " << usedDraw << G4endl;
            usedDraw += 2; // Thinning
        }
    };

}

#endif
