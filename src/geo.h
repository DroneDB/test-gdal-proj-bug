/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef GEO_H
#define GEO_H

#include <iostream>
#include <cmath>

namespace ddb
{

    #ifndef M_PI
    #define M_PI 3.1415926535
    #endif

    // Convert degrees to radians
    inline double deg2rad(double degrees) {
        return degrees * M_PI / 180.0;
    }

    // Convert radians to degrees
    inline double rad2deg(double radians) {
        return radians * 180.0 / M_PI;
    }

    template <typename T>
    struct Projected2D_t
    {
        T x;
        T y;
        Projected2D_t() : x(0.0), y(0.0) {};
        Projected2D_t(T x, T y) : x(x), y(y) {};

        void rotate(const Projected2D_t &center, double degrees)
        {
            double px = this->x;
            double py = this->y;
            double radians = deg2rad(degrees);
            x = cos(radians) * (px - center.x) - sin(radians) * (py - center.y) + center.x;
            y = sin(radians) * (px - center.x) + cos(radians) * (py - center.y) + center.y;
        }
    };

    typedef Projected2D_t<double> Projected2D;
    typedef Projected2D_t<double> Point2D;
    typedef Projected2D_t<int> Projected2Di;

    struct Geographic2D
    {
        double latitude;
        double longitude;
        Geographic2D() : latitude(0.0), longitude(0.0) {};
        Geographic2D(double latitude, double longitude) : latitude(latitude), longitude(longitude) {};
    };

    template <typename T>
    struct BoundingBox
    {
        T min;
        T max;
        BoundingBox() {};
        BoundingBox(const T &min, const T &max) : min(min), max(max) {};

        bool contains(const T &p)
        {
            return contains(p.x, p.y);
        }

        template <typename N>
        bool contains(N x, N y)
        {
            return x >= min.x && x <= max.x && y >= min.y && y <= max.y;
        }
    };

    // Stream operators outside namespace
    inline std::ostream &operator<<(std::ostream &os, const Projected2D &p)
    {
        os << "[" << p.x << ", " << p.y << "]";
        return os;
    }

    template <typename T>
    inline std::ostream &operator<<(std::ostream &os, const BoundingBox<T> &b)
    {
        os << "[" << b.min << "],[" << b.max << "]";
        return os;
    }

    inline std::ostream &operator<<(std::ostream &os, const Geographic2D &p)
    {
        os << "[" << p.latitude << ", " << p.longitude << "]";
        return os;
    }

} // namespace ddb

#endif // GEO_H
