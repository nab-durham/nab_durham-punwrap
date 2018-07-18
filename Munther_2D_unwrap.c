//This program is written by Munther Gdeisat to program the two-dimensional 
//unwrapper entitled "Fast two-dimensional phase-unwrapping algorithm based 
//on sorting by reliability following a noncontinuous path" by  Miguel 
//Arevallilo Herra� ez, David R. Burton, Michael J. Lalor, and Munther A. 
//Gdeisat published in Applied Optics, Vol. 41, No. 35, pp. 7437, 2002.

//This program is written by Munther Gdeisat, Liverpool John Moores 
//University, United Kingdom. Date 26th August 2007

//This program is written to UC Berkeley and it takes consideration of 
//the wrap around problem in the MRI

//Date: August 29 2007
//Program modified by D. Sheltraw so that it can be used as a library. 
//The function phase_unwrap_2D is the principal function called by the 
//user of the library. One can compile this code with:
//
//   gcc -Wall -g -o  Munther_2D_unwrap Munther_2D_unwrap.c -lm
//
//or build the library with
//
//
//The prototype of gcc phase_unwrap_2D is:
//
//   int phase_unwrap_2D(float* WrappedImage, float* UnwrappedImage, 
//                       BYTE* input_mask, int n_pe, int n_fe)
//
//where WrappedImage is a 1D array containing the 2D wrapped data, 
//UnwrappedImage is a 1D array containing the 2D unwrapped data, input_mask
//is a 1D array containing the 2D mask (255 at good points, 0 at bad) n_pe 
//is the number of phase-encoding lines, and n_fe is the number of frequency
//-encoding points. 

#include "Munther_2D_unwrap.h"

// malloc.h is obsolete, stdlib.h is used now (MJT)
//#ifdef DARWIN
//#include <malloc/malloc.h>
//#else
//#include <malloc.h>
//#endif // ifdef DARWIN

#include <stdlib.h>
#include <stdio.h> 
#include <math.h> 
#include <string.h>


static float PI = 3.141592654;
static float TWOPI = 6.283185307;
int x_connectivity_2D = 1;
int y_connectivity_2D = 1;
int No_of_edges = 0;


//---------------start quicker_sort algorithm --------------------------------
#define swap(x,y) {EDGE t; t=x; x=y; y=t;}
#define order(x,y) if (x.reliab > y.reliab) swap(x,y)
#define o2(x,y) order(x,y)
#define o3(x,y,z) o2(x,y); o2(x,z); o2(y,z)

//typedef enum {yes, no} yes_no;

yes_no find_pivot(EDGE *left, EDGE *right, float *pivot_ptr)
{
        /* these need to all be pointers so that when (a,b,c) are aranged
	   in order, the edge list is also updated (MJT)
	*/
        EDGE *a, *b, *c, *p;
	/* added base case check (MJT) */
	if(left >= right) {
	  return no;
	}
	a = left;
	b = left + (right - left)/2 ;
	c = right;
	o3((*a),(*b),(*c));
	if (a->reliab < b->reliab)
	{
		*pivot_ptr = b->reliab;
		return yes;
	}

	if (b->reliab < c->reliab)
	{
		*pivot_ptr = c->reliab;
		return yes;
	}
	/* already know that left == mid == right, so start
	   searching in between left and right for a pivot (MJT) */
	for (p = left+1; p < right; p++)
	{
		if (p->reliab != left->reliab)
		{
		  /* order the pointers first, then return the greater (MJT) */
		  o2((*left), (*p));
		  *pivot_ptr = p->reliab;
		  return yes;
		}
	}
        return no; // DJS
}

EDGE *partition(EDGE *left, EDGE *right, float pivot)
{
  /* It is guaranteed that left->reliab < pivot <= right->reliab,
     so left++ will never pass right..

     however right-- might pass left if left==pivot, but will not pass
     left's first location, since the reliab there is strictly less than pivot
     (MJT)
  */
        while (left <= right)
	{
		while (left->reliab < pivot)
		  ++left;
		while (right->reliab >= pivot)
		  --right;
		if (left < right)
		{
		  swap ((*left), (*right));
		  ++left;
		  --right;
		}
	}
	return left;
}

void quicker_sort(EDGE *left, EDGE *right)
{
	EDGE *p;
	float pivot;
	if (find_pivot(left, right, &pivot) == yes)
	  {
	    p = partition(left, right, pivot);
	    quicker_sort(left, p - 1);
	    quicker_sort(p, right);
	  }
}

//--------------end quicker_sort algorithm -----------------------------------

//--------------------start initialse pixels ----------------------------------
//initialse pixels. See the explination of the pixel class above.
//initially every pixel is a group by its self
void  initialisePIXELs(float *WrappedImage, BYTE *input_mask, BYTE *extended_mask, PIXELM *pixel, int image_width, int image_height)
{
  PIXELM *pixel_pointer = pixel;
  float *wrapped_image_pointer = WrappedImage;
  BYTE *input_mask_pointer = input_mask;
  BYTE *extended_mask_pointer = extended_mask;
  int i, j;

  for (i=0; i < image_height; i++){
    for (j=0; j < image_width; j++){
      //pixel_pointer->x = j;
      //pixel_pointer->y = i;
      pixel_pointer->increment = 0;
      pixel_pointer->number_of_pixels_in_group = 1;		
      pixel_pointer->value = *wrapped_image_pointer;
      pixel_pointer->reliability = (float) (9999999 + rand());
      pixel_pointer->input_mask = *input_mask_pointer;
      pixel_pointer->extended_mask = *extended_mask_pointer;
      pixel_pointer->head = pixel_pointer;
      pixel_pointer->last = pixel_pointer;
      pixel_pointer->next = NULL;			
      pixel_pointer->new_group = 0;
      pixel_pointer->group = -1;
      pixel_pointer++;
      wrapped_image_pointer++;
      input_mask_pointer++;
      extended_mask_pointer++;
    }
  }
}
//-------------------end initialise pixels -----------

//gamma finction in the paper
float wrap(float pixel_value)
{
	float wrapped_pixel_value;
	if (pixel_value > PI)	wrapped_pixel_value = pixel_value - TWOPI;
	else if (pixel_value < -PI)	wrapped_pixel_value = pixel_value + TWOPI;
	else wrapped_pixel_value = pixel_value;
	return wrapped_pixel_value;
}

// pixelL_value is the left pixel,	pixelR_value is the right pixel
int find_wrap(float pixelL_value, float pixelR_value)
{
	float difference; 
	int wrap_value;
	difference = pixelL_value - pixelR_value;

	if (difference > PI)	wrap_value = -1;
	else if (difference < -PI)	wrap_value = 1;
	else wrap_value = 0;

	return wrap_value;
} 

void extend_mask(BYTE *input_mask, BYTE *extended_mask, int image_width, int image_height)
{
	int i,j;
	int image_width_plus_one = image_width + 1;
	int image_width_minus_one = image_width - 1;
	BYTE *IMP = input_mask    + image_width + 1;	//input mask pointer
	BYTE *EMP = extended_mask + image_width + 1;	//extended mask pointer

	//extend the mask for the image except borders
	for (i=1; i < image_height - 1; ++i)
	{
		for (j=1; j < image_width - 1; ++j)
		{
			if ( (*IMP) == 255 && (*(IMP + 1) == 255) && (*(IMP - 1) == 255) && 
				(*(IMP + image_width) == 255) && (*(IMP - image_width) == 255) &&
				(*(IMP - image_width_minus_one) == 255) && (*(IMP - image_width_plus_one) == 255) &&
				(*(IMP + image_width_minus_one) == 255) && (*(IMP + image_width_plus_one) == 255) )
			{		
				*EMP = 255;
			}
			++EMP;
			++IMP;
		}
		EMP += 2;
		IMP += 2;
	}

	if (x_connectivity_2D == 0)
	{
		//extend the mask for the left border of the image
		IMP = input_mask    + image_width;
		EMP = extended_mask + image_width;
		for (i=1; i < image_height - 1; ++ i)
		{
			if ( (*IMP) == 255 && (*(IMP + 1) == 255) && 
				(*(IMP + image_width) == 255) && (*(IMP - image_width) == 255) &&
				(*(IMP - image_width_minus_one) == 255) && (*(IMP + image_width_plus_one) == 255) )
			{		
				*EMP = 255;
			}
			EMP += image_width;
			IMP += image_width;
		}

		//extend the mask for the right border of the image
		IMP = input_mask    + 2 * image_width - 1;
		EMP = extended_mask + 2 * image_width - 1;
		for (i=1; i < image_height - 1; ++i)
		{
			if ( (*IMP) == 255 && (*(IMP - 1) == 255) && 
				(*(IMP + image_width) == 255) && (*(IMP - image_width) == 255) &&
				(*(IMP - image_width_plus_one) == 255) && (*(IMP + image_width_minus_one) == 255) )
			{		
				*EMP = 255;
			}
			EMP += image_width;
			IMP += image_width;
		}
	}

	if (y_connectivity_2D == 0)
	{
		//extend the mask for the top border of the image
		IMP = input_mask    + 1;
		EMP = extended_mask + 1;
		for (i=1; i < image_width - 1; ++i)
		{
			if ( (*IMP) == 255 && (*(IMP - 1) == 255) && (*(IMP + 1) == 255) && 
				(*(IMP + image_width) == 255) && 
				(*(IMP + image_width_plus_one) == 255) && (*(IMP + image_width_minus_one) == 255) )
			{		
				*EMP = 255;
			}
			EMP++;
			IMP++;
		}

		//extend the mask for the bottom border of the image
		IMP = input_mask    + image_width * (image_height - 1) + 1;
		EMP = extended_mask + image_width * (image_height - 1) + 1;
		for (i=1; i < image_width - 1; ++i)
		{
			if ( (*IMP) == 255 && (*(IMP - 1) == 255) && (*(IMP + 1) == 255) &&
				(*(IMP - image_width) == 255) &&
				(*(IMP - image_width_plus_one) == 255) && (*(IMP - image_width_minus_one) == 255) )
			{		
				*EMP = 255;
			}
			EMP++;
			IMP++;
		}
	}		

	if (x_connectivity_2D == 1)
	{
		//extend the mask for the right border of the image
		IMP = input_mask    + 2 * image_width - 1;
		EMP = extended_mask + 2 * image_width -1;
		for (i=1; i < image_height - 1; ++ i)
		{
			if ( (*IMP) == 255 && (*(IMP - 1) == 255) &&  (*(IMP + 1) == 255) &&
				(*(IMP + image_width) == 255) && (*(IMP - image_width) == 255) &&
				(*(IMP - image_width - 1) == 255) && (*(IMP - image_width + 1) == 255) &&
				(*(IMP + image_width - 1) == 255) && (*(IMP - 2 * image_width + 1) == 255) )
			{		
				*EMP = 255;
			}
			EMP += image_width;
			IMP += image_width;
		}

		//extend the mask for the left border of the image
		IMP = input_mask    + image_width;
		EMP = extended_mask + image_width;
		for (i=1; i < image_height - 1; ++i)
		{
			if ( (*IMP) == 255 && (*(IMP - 1) == 255) && (*(IMP + 1) == 255) && 
				(*(IMP + image_width) == 255) && (*(IMP - image_width) == 255) &&
				(*(IMP - image_width + 1) == 255) && (*(IMP + image_width + 1) == 255) &&
				(*(IMP + image_width - 1) == 255) && (*(IMP + 2 * image_width - 1) == 255) )
			{		
				*EMP = 255;
			}
			EMP += image_width;
			IMP += image_width;
		}
	}

	if (y_connectivity_2D == 1)
	{
		//extend the mask for the top border of the image
		IMP = input_mask    + 1;
		EMP = extended_mask + 1;
		for (i=1; i < image_width - 1; ++i)
		{
			if ( (*IMP) == 255 && (*(IMP - 1) == 255) && (*(IMP + 1) == 255) && 
				(*(IMP + image_width) == 255) && (*(IMP + image_width * (image_height - 1)) == 255) &&
				(*(IMP + image_width + 1) == 255) && (*(IMP + image_width - 1) == 255) &&
				(*(IMP + image_width * (image_height - 1) - 1) == 255) && (*(IMP + image_width * (image_height - 1) + 1) == 255) )
			{	
				*EMP = 255;
			}
			EMP++;
			IMP++;
		}

		//extend the mask for the bottom border of the image
		IMP = input_mask    + image_width * (image_height - 1) + 1;
		EMP = extended_mask + image_width * (image_height - 1) + 1;
		for (i=1; i < image_width - 1; ++i)
		{
			if ( (*IMP) == 255 && (*(IMP - 1) == 255) && (*(IMP + 1) == 255) && 
				(*(IMP - image_width) == 255) && (*(IMP - image_width - 1) == 255) && (*(IMP - image_width + 1) == 255) &&
				(*(IMP - image_width * (image_height - 1)    ) == 255) && 
				(*(IMP - image_width * (image_height - 1) - 1) == 255) && 
				(*(IMP - image_width * (image_height - 1) + 1) == 255) )
			{			
				*EMP = 255;
			}
			EMP++;
			IMP++;
		}
	}		
}

void calculate_reliability(float *wrappedImage, PIXELM *pixel, int image_width, int image_height)
{
	int image_width_plus_one = image_width + 1;
	int image_width_minus_one = image_width - 1;
	PIXELM *pixel_pointer = pixel + image_width_plus_one;
	float *WIP = wrappedImage + image_width_plus_one; //WIP is the wrapped image pointer
	float H, V, D1, D2;
	int i, j;
	
	for (i = 1; i < image_height -1; ++i)
	{
		for (j = 1; j < image_width - 1; ++j)
		{
			if (pixel_pointer->extended_mask == 255)
			{
				H = wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP + 1));
				V = wrap(*(WIP - image_width) - *WIP) - wrap(*WIP - *(WIP + image_width));
				D1 = wrap(*(WIP - image_width_plus_one) - *WIP) - wrap(*WIP - *(WIP + image_width_plus_one));
				D2 = wrap(*(WIP - image_width_minus_one) - *WIP) - wrap(*WIP - *(WIP + image_width_minus_one));
				pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;
			}
			pixel_pointer++;
			WIP++;
		}
		pixel_pointer += 2;
		WIP += 2;
	}

	if (x_connectivity_2D == 1)
	{
		//calculating the raliability for the left border of the image
		pixel_pointer = pixel + image_width;
		WIP = wrappedImage + image_width; 
	
		for (i = 1; i < image_height - 1; ++i)
		{
			if (pixel_pointer->extended_mask == 255)
			{
				H = wrap(*(WIP + image_width - 1) - *WIP) - wrap(*WIP - *(WIP + 1));
				V = wrap(*(WIP - image_width) - *WIP) - wrap(*WIP - *(WIP + image_width));
				D1 = wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP + image_width_plus_one));
				D2 = wrap(*(WIP - image_width_minus_one) - *WIP) - wrap(*WIP - *(WIP + 2* image_width - 1));
				pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;
			}
			pixel_pointer += image_width;
			WIP += image_width;
		}

		//calculating the raliability for the right border of the image
		pixel_pointer = pixel + 2 * image_width - 1;
		WIP = wrappedImage + 2 * image_width - 1; 
	
		for (i = 1; i < image_height - 1; ++i)
		{
			if (pixel_pointer->extended_mask == 255)
			{
				H = wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP - image_width_minus_one));
				V = wrap(*(WIP - image_width) - *WIP) - wrap(*WIP - *(WIP + image_width));
				D1 = wrap(*(WIP - image_width_plus_one) - *WIP) - wrap(*WIP - *(WIP + 1));
				D2 = wrap(*(WIP - (2 * image_width - 1)) - *WIP) - wrap(*WIP - *(WIP + image_width_minus_one));
				pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;
			}
			pixel_pointer += image_width;
			WIP += image_width;
		}
	}

	if (y_connectivity_2D == 1)
	{
		//calculating the raliability for the top border of the image
		pixel_pointer = pixel + 1;
		WIP = wrappedImage + 1; 
	
		for (i = 1; i < image_width - 1; ++i)
		{
			if (pixel_pointer->extended_mask == 255)
			{
				H =  wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP + 1));
				V =  wrap(*(WIP + image_width*(image_height - 1)) - *WIP) - wrap(*WIP - *(WIP + image_width));
				D1 = wrap(*(WIP + image_width*(image_height - 1) - 1) - *WIP) - wrap(*WIP - *(WIP + image_width_plus_one));
				D2 = wrap(*(WIP + image_width*(image_height - 1) + 1) - *WIP) - wrap(*WIP - *(WIP + image_width_minus_one));
				pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;
			}
			pixel_pointer++;
			WIP++;
		}

		//calculating the raliability for the bottom border of the image
		pixel_pointer = pixel + (image_height - 1) * image_width + 1;
		WIP = wrappedImage + (image_height - 1) * image_width + 1; 
	
		for (i = 1; i < image_width - 1; ++i)
		{
			if (pixel_pointer->extended_mask == 255)
			{
				H =  wrap(*(WIP - 1) - *WIP) - wrap(*WIP - *(WIP + 1));
				V =  wrap(*(WIP - image_width) - *WIP) - wrap(*WIP - *(WIP -(image_height - 1) * (image_width)));
				D1 = wrap(*(WIP - image_width_plus_one) - *WIP) - wrap(*WIP - *(WIP - (image_height - 1) * (image_width) + 1));
				D2 = wrap(*(WIP - image_width_minus_one) - *WIP) - wrap(*WIP - *(WIP - (image_height - 1) * (image_width) - 1));
				pixel_pointer->reliability = H*H + V*V + D1*D1 + D2*D2;
			}
			pixel_pointer++;
			WIP++;
		}
	}
}

//calculate the reliability of the horizental edges of the image
//it is calculated by adding the reliability of pixel and the relibility of 
//its right neighbour
//edge is calculated between a pixel and its next neighbour
void  horizentalEDGEs(PIXELM *pixel, EDGE *edge, int image_width, int image_height)
{
	int i, j;
	EDGE *edge_pointer = edge;
	PIXELM *pixel_pointer = pixel;
	
	for (i = 0; i < image_height; i++)
	{
		for (j = 0; j < image_width - 1; j++) 
		{
			if (pixel_pointer->input_mask == 255 && (pixel_pointer + 1)->input_mask == 255)
			{
				edge_pointer->pointer_1 = pixel_pointer;
				edge_pointer->pointer_2 = (pixel_pointer+1);
				edge_pointer->reliab = pixel_pointer->reliability + (pixel_pointer + 1)->reliability;
				edge_pointer->increment = find_wrap(pixel_pointer->value, (pixel_pointer + 1)->value);
				edge_pointer++;
				No_of_edges++;
			}
			pixel_pointer++;
		}
		pixel_pointer++;
	}
	//construct edges at the right border of the image
	if (x_connectivity_2D == 1)
	{
		pixel_pointer = pixel + image_width - 1;
		for (i = 0; i < image_height; i++)
		{
			if (pixel_pointer->input_mask == 255 && (pixel_pointer - image_width + 1)->input_mask == 255)
			{
				edge_pointer->pointer_1 = pixel_pointer;
				edge_pointer->pointer_2 = (pixel_pointer - image_width + 1);
				edge_pointer->reliab = pixel_pointer->reliability + (pixel_pointer - image_width + 1)->reliability;
				edge_pointer->increment = find_wrap(pixel_pointer->value, (pixel_pointer  - image_width + 1)->value);
				edge_pointer++;
				No_of_edges++;
			}
			pixel_pointer+=image_width;
		}
	}
}

//calculate the reliability of the vertical edges of the image
//it is calculated by adding the reliability of pixel and the relibility of 
//its lower neighbour in the image.
void  verticalEDGEs(PIXELM *pixel, EDGE *edge, int image_width, int image_height)
{
	int i, j;
	PIXELM *pixel_pointer = pixel;
	EDGE *edge_pointer = edge + No_of_edges; 

	for (i=0; i < image_height - 1; i++)
	{
		for (j=0; j < image_width; j++) 
		{
			if (pixel_pointer->input_mask == 255 && (pixel_pointer + image_width)->input_mask == 255)
			{
				edge_pointer->pointer_1 = pixel_pointer;
				edge_pointer->pointer_2 = (pixel_pointer + image_width);
				edge_pointer->reliab = pixel_pointer->reliability + (pixel_pointer + image_width)->reliability;
				edge_pointer->increment = find_wrap(pixel_pointer->value, (pixel_pointer + image_width)->value);
				edge_pointer++;
				No_of_edges++;
			}
			pixel_pointer++;
		} //j loop
	} // i loop

	//construct edges that connect at the bottom border of the image
	if (y_connectivity_2D == 1)
	{
		pixel_pointer = pixel + image_width *(image_height - 1);
		for (i = 0; i < image_width; i++)
		{
			if (pixel_pointer->input_mask == 255 && (pixel_pointer - image_width *(image_height - 1))->input_mask == 255)
			{
				edge_pointer->pointer_1 = pixel_pointer;
				edge_pointer->pointer_2 = (pixel_pointer - image_width *(image_height - 1));
				edge_pointer->reliab = pixel_pointer->reliability + (pixel_pointer - image_width *(image_height - 1))->reliability;
				edge_pointer->increment = find_wrap(pixel_pointer->value, (pixel_pointer - image_width *(image_height - 1))->value);
				edge_pointer++;
				No_of_edges++;
			}
			pixel_pointer++;
		}
	}
}

//gather the pixels of the image into groups 
void  gatherPIXELs(EDGE *edge, int image_width, int image_height)
{
	int k;
	PIXELM *PIXEL1;   
	PIXELM *PIXEL2;
	PIXELM *group1;
	PIXELM *group2;
	EDGE *pointer_edge = edge;
	int incremento;

	for (k = 0; k < No_of_edges; k++)
	{
		PIXEL1 = pointer_edge->pointer_1;
		PIXEL2 = pointer_edge->pointer_2;

		//PIXELM 1 and PIXELM 2 belong to different groups
		//initially each pixel is a group by it self and one pixel can construct a group
		//no else or else if to this if
		if (PIXEL2->head != PIXEL1->head)
		{
			//PIXELM 2 is alone in its group
			//merge this pixel with PIXELM 1 group and find the number of 2 pi to add 
			//to or subtract to unwrap it
			if ((PIXEL2->next == NULL) && (PIXEL2->head == PIXEL2))
			{
				PIXEL1->head->last->next = PIXEL2;
				PIXEL1->head->last = PIXEL2;
				(PIXEL1->head->number_of_pixels_in_group)++;
				PIXEL2->head=PIXEL1->head;
				PIXEL2->increment = PIXEL1->increment-pointer_edge->increment;
			}

			//PIXELM 1 is alone in its group
			//merge this pixel with PIXELM 2 group and find the number of 2 pi to add 
			//to or subtract to unwrap it
			else if ((PIXEL1->next == NULL) && (PIXEL1->head == PIXEL1))
			{
				PIXEL2->head->last->next = PIXEL1;
				PIXEL2->head->last = PIXEL1;
				(PIXEL2->head->number_of_pixels_in_group)++;
				PIXEL1->head = PIXEL2->head;
				PIXEL1->increment = PIXEL2->increment+pointer_edge->increment;
			} 

			//PIXELM 1 and PIXELM 2 both have groups
			else
            {
				group1 = PIXEL1->head;
                group2 = PIXEL2->head;
				//the no. of pixels in PIXELM 1 group is large than the no. of pixels
				//in PIXELM 2 group.   Merge PIXELM 2 group to PIXELM 1 group
				//and find the number of wraps between PIXELM 2 group and PIXELM 1 group
				//to unwrap PIXELM 2 group with respect to PIXELM 1 group.
				//the no. of wraps will be added to PIXELM 2 grop in the future
				if (group1->number_of_pixels_in_group > group2->number_of_pixels_in_group)
				{
					//merge PIXELM 2 with PIXELM 1 group
					group1->last->next = group2;
					group1->last = group2->last;
					group1->number_of_pixels_in_group = group1->number_of_pixels_in_group + group2->number_of_pixels_in_group;
					incremento = PIXEL1->increment-pointer_edge->increment - PIXEL2->increment;
					//merge the other pixels in PIXELM 2 group to PIXELM 1 group
					while (group2 != NULL)
					{
						group2->head = group1;
						group2->increment += incremento;
						group2 = group2->next;
					}
				} 

				//the no. of pixels in PIXELM 2 group is large than the no. of pixels
				//in PIXELM 1 group.   Merge PIXELM 1 group to PIXELM 2 group
				//and find the number of wraps between PIXELM 2 group and PIXELM 1 group
				//to unwrap PIXELM 1 group with respect to PIXELM 2 group.
				//the no. of wraps will be added to PIXELM 1 grop in the future
				else
                {
					//merge PIXELM 1 with PIXELM 2 group
					group2->last->next = group1;
					group2->last = group1->last;
					group2->number_of_pixels_in_group = group2->number_of_pixels_in_group + group1->number_of_pixels_in_group;
					incremento = PIXEL2->increment + pointer_edge->increment - PIXEL1->increment;
					//merge the other pixels in PIXELM 2 group to PIXELM 1 group
					while (group1 != NULL)
					{
						group1->head = group2;
						group1->increment += incremento;
						group1 = group1->next;
					} // while

                } // else
            } //else
        } //if
        pointer_edge++;
	}
}

//unwrap the image 
void  unwrapImage(PIXELM *pixel, int image_width, int image_height)
{
	int i;
	int image_size = image_width * image_height;
	PIXELM *pixel_pointer=pixel;

	for (i = 0; i < image_size; i++)
	{
		pixel_pointer->value += TWOPI * (float)(pixel_pointer->increment);
        pixel_pointer++;
    }
}

//set the masked pixels (mask = 0) to the minimum of the unwrapper phase
void  maskImage(PIXELM *pixel, BYTE *input_mask, int image_width, int image_height)
{
	/*int image_width_plus_one  = image_width + 1;
	int image_height_plus_one  = image_height + 1;
	int image_width_minus_one = image_width - 1;
	int image_height_minus_one = image_height - 1;*/

	PIXELM *pointer_pixel = pixel;
	BYTE *IMP = input_mask;	//input mask pointer
	float min=99999999.;
	int i; 
	int image_size = image_width * image_height;

	//find the minimum of the unwrapped phase
	for (i = 0; i < image_size; i++)
	{
		if ((pointer_pixel->value < min) && (*IMP == 255)) 
			min = pointer_pixel->value;

		pointer_pixel++;
		IMP++;
	}

	pointer_pixel = pixel;
	IMP = input_mask;	

	//set the masked pixels to minimum
	for (i = 0; i < image_size; i++)
	{
		if ((*IMP) == 0)
		{
			pointer_pixel->value = min;
		}
		pointer_pixel++;
		IMP++;
	}
}

//the input to this unwrapper is an array that contains the wrapped phase map. 
//copy the image on the buffer passed to this unwrapper to over write the 
//unwrapped phase map on the buffer of the wrapped phase map.
void returnImage(PIXELM *pixel, float *unwrappedImage, int image_width, 
                  int image_height)
{
  int i;
  int image_size = image_width * image_height;
  float *unwrappedImage_pointer = unwrappedImage;
  PIXELM *pixel_pointer = pixel;

  for(i=0; i < image_size; i++){
    *unwrappedImage_pointer = pixel_pointer->value;
    pixel_pointer++;
    unwrappedImage_pointer++;
  }
}

// Scan the mask quickly to determine whether it will crash the program.
// Two contiguous (vertical or horizontal) unmasked points should be enough
// (added by MJT)
int isSaneMask(BYTE* input_mask, int n_pe, int n_fe)
{
  int l, m;
  BYTE *mp;
  mp = input_mask;
  for(l=0; l<n_pe; l++) {
    for(m=0; m<n_fe; m++) {
      if (!(*mp == 255)) {
	  mp++;
	  continue;
      }
      
      if ((m < n_fe-1) && (l < n_pe-1)) {
	if (*(mp+1) == 255 || *(mp+n_fe)==255)
	  return 1;
      }
      else if (m < n_fe-1) {
	if (*(mp+1) == 255)
	  return 1;
      }
      else if (l < n_pe-1) {
	if (*(mp+n_fe) == 255)
	  return 1;
      }
      mp++;
    }
  }
  return 0;
}

int phase_unwrap_2D(float* WrappedImage, float* UnwrappedImage, 
                    BYTE* input_mask, int n_pe, int n_fe)  
{  
  BYTE *extended_mask;
  PIXELM *pixel;
  EDGE *edge;
  int image_size;
  int No_of_Edges_initially;
  int k;
  image_size = n_pe * n_fe;
  No_of_Edges_initially = 2* n_pe * n_fe; 
  //Allocate some memory for internal arrays.
  extended_mask = (BYTE *) calloc(image_size, sizeof(BYTE));
  pixel = (PIXELM *) calloc(image_size, sizeof(PIXELM));
  edge = (EDGE *) calloc(No_of_Edges_initially, sizeof(EDGE));

  if(input_mask==NULL) {
    input_mask = (BYTE *) calloc(image_size, sizeof(BYTE));
    for(k=0; k<image_size; k++) *(input_mask+k) = 255;
  }
  // if the mask is insane, then no unwrapping will happen (MJT)
  if (!isSaneMask(input_mask, n_pe, n_fe)) {
    memmove(UnwrappedImage, WrappedImage, n_pe*n_fe*sizeof(float));
    return 0;
  }
  extend_mask(input_mask, extended_mask, n_fe, n_pe);
  initialisePIXELs(WrappedImage, input_mask, extended_mask, pixel, n_fe, n_pe);
  calculate_reliability(WrappedImage, pixel, n_fe, n_pe);
  horizentalEDGEs(pixel, edge, n_fe, n_pe); 
  verticalEDGEs(pixel, edge, n_fe, n_pe);
  //Sort the EDGEs depending on their reiability: PIXELs with higher 
  //relibility (small value) first.
  quicker_sort(edge, edge + No_of_edges - 1);
  //Gather PIXELs into groups
  gatherPIXELs(edge, n_fe, n_pe);
  unwrapImage(pixel, n_fe, n_pe);
  maskImage(pixel, input_mask, n_fe, n_pe);

  //Copy the image from PIXELM structure to the unwrapped phase array passed 
  //to this function.
  returnImage(pixel, UnwrappedImage, n_fe, n_pe);
  //Free memory for internal arrays.
  free(edge);
  free(pixel);
  free(extended_mask);

  // restoring global counter (MJT)
  No_of_edges = 0;
  
  return 1;
}
