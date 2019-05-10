#ifndef _ACER_COLOR_ENGINE_MTK_H_
#define _ACER_COLOR_ENGINE_MTK_H_

void acer_color_initial(void);
int acer_color_analysis_histogram( unsigned long *His, int count);
int acer_color_luma_smooth(void);
int acer_sunlight_readable_luma_smooth(void);
int acer_ALS_analysis_function(int current_ALS_value);
int acer_sunlight_content_analysis_function(int content_aware_profile, int sunlight_readable_profile);
void acer_sunlight_content_smooth(int *, int *);

#endif //_ACER_COLOR_ENGINE_MTK_H
