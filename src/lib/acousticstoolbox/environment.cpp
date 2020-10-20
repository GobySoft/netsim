// Copyright 2020:
//   GobySoft, LLC (2017-)
//   Massachusetts Institute of Technology (2017-)
// File authors:
//   Toby Schneider <toby@gobysoft.org>
//
//
// This file is part of the NETSIM Libraries.
//
// The NETSIM Libraries are free software: you can redistribute them and/or modify
// them under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 2.1 of the License, or
// (at your option) any later version.
//
// The NETSIM Libraries are distributed in the hope that they will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with NETSIM.  If not, see <http://www.gnu.org/licenses/>.

#include <boost/assign.hpp>
#include <boost/foreach.hpp>
#include <iomanip>
#include <iostream>

#include "environment.h"

void netsim::bellhop::Environment::output_env(std::ostream* os, std::ostream* ssp,
                                              std::ostream* bty, std::ostream* trc,
                                              std::ostream* brc, const protobuf::Environment& env)
{
    const double BOX_BUFFER_PERCENT =
        10; // how much to grow the calculation box from the actual sources

    *os << "'" << env.title() << "'" << std::endl;
    *os << env.freq() << std::endl;
    *os << "1" << std::endl; // NMEDIA (unused)

    *os << "'";

    if (env.water_column_size() > 1)
        *os << 'Q';
    else
    {
        switch (env.water_column(0).interpolation_type())
        {
            case protobuf::Environment::WaterColumn::CUBIC_SPLINE: *os << 'S'; break;
            case protobuf::Environment::WaterColumn::C_LINEAR: *os << 'C'; break;
            case protobuf::Environment::WaterColumn::N2_LINEAR: *os << 'N'; break;
            default: break;
        }
    }

    if (env.surface().medium().type() == protobuf::Environment::Medium::HALF_SPACE)
        *os << env.surface().medium().depth() << " " << env.surface().medium().cp() << " "
            << env.surface().medium().cs() << " " << env.surface().medium().density() << " "
            << env.surface().medium().attenuation().value() << " /" << std::endl;
    else if (env.surface().medium().type() == protobuf::Environment::Medium::REFLECTION_COEFFICIENT)
    {
        if (env.surface().medium().rc_sample_size() > 0)
        {
            *trc << env.surface().medium().rc_sample_size() << std::endl;
            for (int i = 0; i < env.surface().medium().rc_sample_size(); i++)
            {
                *trc << env.surface().medium().rc_sample(i).angle() << ' ';
                *trc << pow(10., -env.surface().medium().rc_sample(i).rc() / 20) << ' ';
                *trc << env.surface().medium().rc_sample(i).phase() << std::endl;
            }
        }
    }
    switch (env.surface().medium().type())
    {
        case protobuf::Environment::Medium::VACUUM: *os << 'V'; break;
        case protobuf::Environment::Medium::RIGID: *os << 'R'; break;
        case protobuf::Environment::Medium::HALF_SPACE: *os << 'A'; break;
        case protobuf::Environment::Medium::REFLECTION_COEFFICIENT: *os << 'F'; break;
        default: break;
    }
    switch (env.bottom().medium().attenuation().units())
    {
        case protobuf::Environment::Medium::Attenuation::DB_PER_M_KHZ: *os << 'F'; break;
        case protobuf::Environment::Medium::Attenuation::PARAMETER_LOSS: *os << 'L'; break;
        case protobuf::Environment::Medium::Attenuation::DB_PER_M: *os << 'M'; break;
        case protobuf::Environment::Medium::Attenuation::NEPERS_PER_M: *os << 'N'; break;
        case protobuf::Environment::Medium::Attenuation::Q_FACTOR: *os << 'Q'; break;
        case protobuf::Environment::Medium::Attenuation::DB_PER_WAVELENGTH: *os << 'W'; break;
        default: break;
    }

    double max_depth;
    std::vector<double> bathy;
    std::vector<std::vector<double>> rd_ssp_mat;
    std::vector<int> deepest_ssp_dep;
    std::vector<double> deepest_ssp_cp;

    int k_bathy_max = 0;
    double bathy_max = 0;
    for (int k = 0, wc_size = env.water_column_size(); k < wc_size; k++)
    {
        if (env.water_column(k).bathymetry() > bathy_max)
        {
            bathy_max = env.water_column(k).bathymetry();
            k_bathy_max = k;
        }
    }

    // range-independent case (for .env file)
    // ALSO maximum depth slice of the range-dependent case (k_bathy_max)
    {
        // insert into a map to order the samples ascending
        std::map<double, double> depth_ssp_map;
        int m_coef = env.water_column(k_bathy_max).eof_coef_size();
        std::vector<double> eof_coef;
        eof_coef.resize(m_coef);
        for (int j = 0; j < m_coef; j++) eof_coef[j] = env.water_column(k_bathy_max).eof_coef(j);
        for (int i = 0, n = env.water_column(k_bathy_max).sample_size(); i < n; ++i)
        {
            double depth = env.water_column(k_bathy_max).sample(i).depth();
            double cp = env.water_column(k_bathy_max).sample(i).cp();
            int m = env.water_column(k_bathy_max).sample(i).eof_size();
            double ssp = cp;
            for (int j = 0; j < std::min(m, m_coef); j++)
                ssp += env.water_column(k_bathy_max).sample(i).eof(j) * eof_coef[j];

            depth_ssp_map[env.water_column(k_bathy_max).sample(i).depth()] = ssp;
        }
        // Note that for the RD case, the .env ssp at range 0
        // MUST define max_depth
        std::map<double, double>::iterator back_it = depth_ssp_map.end();
        --back_it;
        max_depth = back_it->first;
        if (env.water_column(k_bathy_max).use_attenuation())
            *os << 'T';
        *os << "'" << std::endl;

        *os << "0 0 " << std::fixed << std::setprecision(3) << max_depth << std::endl;

        for (std::map<double, double>::const_iterator it = depth_ssp_map.begin(),
                                                      end = depth_ssp_map.end();
             it != end; ++it)
        {
            *os << std::fixed << std::setprecision(3) << it->first << " " << it->second << " /"
                << std::endl;
            double z = it->first;
            deepest_ssp_dep.push_back(z);
            deepest_ssp_cp.push_back(it->second);
        }
        *os << "'";
    }

    // range-dependent case
    if (env.water_column_size() > 1)
    {
        for (int k = 0, wc_size = env.water_column_size(); k < wc_size; k++)
        {
            std::map<double, double> depth_ssp_map;

            int m_coef = env.water_column(k).eof_coef_size();
            std::vector<double> eof_coef;
            eof_coef.resize(m_coef);
            for (int j = 0; j < m_coef; j++) eof_coef[j] = env.water_column(k).eof_coef(j);
            for (int i = 0, n = env.water_column(k).sample_size(); i < n; ++i)
            {
                double depth = env.water_column(k).sample(i).depth();
                double cp = env.water_column(k).sample(i).cp();
                int m = env.water_column(k).sample(i).eof_size();
                double ssp = cp;
                for (int j = 0; j < std::min(m, m_coef); j++)
                    ssp += env.water_column(k).sample(i).eof(j) * eof_coef[j];

                depth_ssp_map[env.water_column(k).sample(i).depth()] = ssp;
            }
            // create ssp and bathymetry maps for range-dependent case
            bathy.push_back(env.water_column(k).bathymetry());

            // extend the deepest cp value to fill out the grid
            rd_ssp_mat.push_back(std::vector<double>(env.water_column(k_bathy_max).sample_size(),
                                                     depth_ssp_map.rbegin()->second));

            unsigned indx = 0;
            for (std::map<double, double>::const_iterator it = depth_ssp_map.begin(),
                                                          end = depth_ssp_map.end();
                 it != end; ++it)
            {
                if ((int)it->first != deepest_ssp_dep[indx])
                {
                    if (indx == depth_ssp_map.size() - 1) // deepest value could be different depth
                    {
                        // pin the deepest depth below the sea floor to the correct grid point
                        rd_ssp_mat[k][indx] = it->second;
                        indx++;
                    }
                    else
                    {
                        throw std::runtime_error("SSP grid must have same depths for all ranges");
                    }
                }
                else
                {
                    rd_ssp_mat[k][indx] = it->second;
                    indx++;
                }
            }
        }
    }

    switch (env.bottom().medium().type())
    {
        case protobuf::Environment::Medium::VACUUM: *os << 'V'; break;
        case protobuf::Environment::Medium::RIGID: *os << 'R'; break;
        case protobuf::Environment::Medium::HALF_SPACE: *os << 'A'; break;
        case protobuf::Environment::Medium::REFLECTION_COEFFICIENT: *os << 'F'; break;
        default: break;
    }
    // For RD signal need to read bty file
    if (env.water_column_size() > 1)
        *os << '*';

    *os << "' 0" << std::endl;

    if (env.bottom().medium().type() == protobuf::Environment::Medium::HALF_SPACE)
        *os << env.bottom().medium().depth() << " " << env.bottom().medium().cp() << " "
            << env.bottom().medium().cs() << " " << env.bottom().medium().density() << " "
            << env.bottom().medium().attenuation().value() << " /" << std::endl;
    else if (env.bottom().medium().type() == protobuf::Environment::Medium::REFLECTION_COEFFICIENT)
    {
        if (env.bottom().medium().rc_sample_size() > 0)
        {
            *brc << env.bottom().medium().rc_sample_size() << std::endl;
            for (int i = 0; i < env.bottom().medium().rc_sample_size(); i++)
            {
                *brc << env.bottom().medium().rc_sample(i).angle() << ' ';
                *brc << pow(10., -env.bottom().medium().rc_sample(i).rc() / 20) << ' ';
                *brc << env.bottom().medium().rc_sample(i).phase() << std::endl;
            }
        }
    }

    *os << env.sources().number_in_depth() << std::endl;
    *os << env.sources().first().depth();
    if (env.sources().number_in_depth() > 1)
        *os << " " << env.sources().last().depth();
    *os << " /" << std::endl;

    *os << env.receivers().number_in_depth() << std::endl;
    *os << env.receivers().first().depth();
    if (env.receivers().number_in_depth() > 1)
        *os << " " << env.receivers().last().depth();
    *os << " /" << std::endl;

    *os << env.receivers().number_in_range() << std::endl;
    *os << env.receivers().first().range() / 1000;
    if (env.receivers().number_in_range() > 1)
        *os << " " << env.receivers().last().range() / 1000;
    *os << " /" << std::endl;

    *os << "'";
    switch (env.output().type())
    {
        case protobuf::Environment::Output::ARRIVAL_TIMES: *os << 'A'; break;
        case protobuf::Environment::Output::EIGENRAYS: *os << 'E'; break;
        case protobuf::Environment::Output::RAYS: *os << 'R'; break;
        case protobuf::Environment::Output::COHERENT_PRESSURE: *os << 'C'; break;
        case protobuf::Environment::Output::INCOHERENT_PRESSURE: *os << 'I'; break;
        case protobuf::Environment::Output::SEMICOHERENT_PRESSURE: *os << 'S'; break;
        default: break;
    }
    switch (env.beams().approximation_type())
    {
        case protobuf::Environment::Beams::GEOMETRIC: *os << 'G'; break;
        case protobuf::Environment::Beams::CARTESIAN: *os << 'C'; break;
        case protobuf::Environment::Beams::RAY_CENTERED: *os << 'R'; break;
        case protobuf::Environment::Beams::GAUSSIAN: *os << 'B'; break;
        default: break;
    }
    *os << "'" << std::endl;
    *os << env.beams().number() << std::endl;
    *os << env.beams().theta_min() << " " << env.beams().theta_max() << " /" << std::endl;
    *os << "0 " << (int)(max_depth * (1 + BOX_BUFFER_PERCENT / 100)) << " "
        << (env.receivers().last().range() / 1000) * (1 + BOX_BUFFER_PERCENT / 100) << std::endl;

    if (env.water_column_size() > 1)
    {
        // write ssp and bty files
        *ssp << env.water_column_size() << std::endl;
        *bty << "'L'" << std::endl;
        *bty << env.water_column_size() << std::endl;

        for (int k = 0, m = env.water_column_size(); k < m; k++)
        {
            *ssp << env.water_column(k).range() / 1000 << " ";
            *bty << env.water_column(k).range() / 1000 << " " << std::fixed << std::setprecision(3)
                 << env.water_column(k).bathymetry() << std::endl;
        }
        *ssp << std::endl;

        for (int i = 0, n = rd_ssp_mat[0].size(); i < n; ++i)
        {
            for (int k = 0, m = rd_ssp_mat.size(); k < m; k++) *ssp << rd_ssp_mat[k][i] << " ";
            *ssp << std::endl;
        }
    }
}
