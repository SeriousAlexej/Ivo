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
#ifndef IVO_SERIAL_UTILS_H
#define IVO_SERIAL_UTILS_H
#include <QJsonValue>
#include <QJsonArray>
#include <stdexcept>
#include <vector>
#include <type_traits>
#include <iterator>

//**************************************** WRITING ***********************************
static inline bool ToJSON(const bool& val) { return val; }

template<typename TVal, typename std::enable_if<std::is_integral<TVal>::value || std::is_enum<TVal>::value, int>::type = 0>
int ToJSON(const TVal& val) { return static_cast<int>(val); }

template<typename TVal, typename std::enable_if<std::is_floating_point<TVal>::value, int>::type = 0>
double ToJSON(const TVal& val) { return static_cast<double>(val); }

//write glm type (vec2, uvec4, mat3 etc)
template<typename TVec, typename std::enable_if<std::is_arithmetic<typename TVec::value_type>::value, int>::type = 0>
QJsonArray ToJSON(const TVec& vec)
{
    QJsonArray valArr;
    for(decltype(vec.length()) i=0; i<vec.length(); ++i)
        valArr.append(ToJSON(vec[i]));
    return valArr;
}

//write array
template<typename TArr, typename std::enable_if<std::is_array<TArr>::value, int>::type = 0>
QJsonArray ToJSON(const TArr& arr)
{
    QJsonArray valArr;
    for(auto it = std::begin(arr); it != std::end(arr); it++)
        valArr.append(ToJSON(*it));
    return valArr;
}

//write vector
template<typename TVec>
QJsonArray ToJSON(const std::vector<TVec>& vec)
{
    QJsonArray arr;
    for(const auto& val : vec)
        arr.append(ToJSON(val));
    return arr;
}

//**************************************** READING ***********************************
static inline void FromJSON(const QJsonValue& jVal, bool& val) { val = jVal.toBool(); }

template<typename TVal, typename std::enable_if<std::is_integral<TVal>::value || std::is_enum<TVal>::value, int>::type = 0>
void FromJSON(const QJsonValue& jVal, TVal& val) { val = static_cast<TVal>(jVal.toInt()); }

template<typename TVal, typename std::enable_if<std::is_floating_point<TVal>::value, int>::type = 0>
void FromJSON(const QJsonValue& jVal, TVal& val) { val = static_cast<TVal>(jVal.toDouble()); }

//read glm type (vec2, uvec4, mat3 etc)
template<typename TVec, typename std::enable_if<std::is_arithmetic<typename TVec::value_type>::value, int>::type = 0>
void FromJSON(const QJsonValue& arrVal, TVec& vec)
{
    const QJsonArray arr = arrVal.toArray();
    const int len = arr.size();
    if(len != vec.length())
        throw std::runtime_error("GLM dimensions mismatch!");
    for(int i=0; i<len; ++i)
        FromJSON(arr.at(i), vec[i]);
}

//read array of types
template<typename TArr, typename std::enable_if<std::is_array<TArr>::value, int>::type = 0>
void FromJSON(const QJsonValue& jArrVal, TArr& arr)
{
    const QJsonArray jArr = jArrVal.toArray();
    if(jArr.size() != std::distance(std::begin(arr), std::end(arr)))
        throw std::runtime_error("Array size mismatch!");
    auto it = std::begin(arr);
    for(int i=0; i<jArr.size(); ++i, ++it)
        FromJSON(jArr.at(i), *it);
}

//read vector types
template<typename TVec>
void FromJSON(const QJsonValue& arrVal, std::vector<TVec>& vec)
{
    const QJsonArray arr = arrVal.toArray();
    vec.clear();
    vec.reserve(arr.size());
    for(int i=0; i<arr.size(); ++i)
    {
        vec.emplace_back();
        FromJSON(arr.at(i), vec.back());
    }
}

#endif
