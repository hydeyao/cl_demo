
__kernel void rgb2gray(__global unsigned char* rgb_img,\
					   __global unsigned char* gray_img,\
					   __global unsigned * const p_height,\
					   __global unsigned * const p_width
					   )
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	int width = *p_width;
	int height = *p_height;

	if(x < width && y < height)
	{
		int index = y * height + x;
		 grayImage[index] = 0.299f*rgbImage[index*3] + 
                            0.587f*rgbImage[index*3+1] + 
                            0.114f*rgbImage[index*3+2];
	}
}