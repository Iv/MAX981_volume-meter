//
// Created by iv on 1/2/26.
//

#ifndef MAX981_VOLUME_METER_UTILS_H
#define MAX981_VOLUME_METER_UTILS_H

#define UTILS_LP_FAST(value, sample, filter_constant)	(value -= (filter_constant) * ((value) - (sample)))


#endif //MAX981_VOLUME_METER_UTILS_H