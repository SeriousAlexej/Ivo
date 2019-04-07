/*
    Ivo - a free software for unfolding 3D models and papercrafting
    Copyright (C) 2015-2018 Oleksii Sierov (seriousalexej@gmail.com)
	
    Ivo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ivo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Ivo.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <QString>
#include <unordered_map>
#include <string>

namespace Formats3D
{

QString GetSupported3DFormats()
{
    static std::string supportedFormats = "";
    if(supportedFormats.empty())
    {
        std::unordered_map<std::string, std::string> formats;

        //ADD NEW FORMATS HERE

        formats["Autodesk"]                 = "fbx";
        formats["Collada"]                  = "dae";
        formats["Blender 3D"]               = "blend";
        formats["3ds Max 3DS"]              = "3ds";
        formats["3ds Max ASE"]              = "ase";
        formats["Wavefront Object"]         = "obj";
        formats["Stanford Polygon Library"] = "ply";
        formats["LightWave"]                = "lwo";
        formats["LightWave Scene"]          = "lws";
        formats["DirectX X"]                = "x";
        formats["AC3D"]                     = "ac";
        formats["Milkshape 3D"]             = "ms3d";
        formats["Ogre XML"]                 = "xml";
        formats["Irrlicht Mesh"]            = "irrmesh";
        formats["Quake I"]                  = "mdl";
        formats["Quake II"]                 = "md2";
        formats["Quake III Mesh"]           = "md3";
        formats["BlitzBasic 3D"]            = "b3d";
        formats["Neutral File Format"]      = "nff";
        formats["Sense8 WorldToolKit"]      = "nff";
        formats["Object File Format"]       = "off";


        //NOW GENERATE FORMATS STRING FROM DATA ABOVE

        supportedFormats = "All supported formats (";
        for(auto it=formats.begin(); it!=formats.end(); it++)
        {
            if(it!=formats.begin())
                supportedFormats += " ";
            supportedFormats += "*." + it->second;
        }
        supportedFormats += ")";
        for(auto it=formats.begin(); it!=formats.end(); it++)
        {
            supportedFormats += ";;" + it->first + " (*." + it->second + ")";
        }
    }
    return supportedFormats.c_str();
}

}//namespace Formats3D
