#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <algorithm>
#include <epoxy/gl.h>
#include <gtest/gtest.h>
#include <gtest/gtest-message.h>

#include "flat_input.h"
#include "init.h"
#include "resource_pool.h"
#include "test_util.h"
#include "util.h"

using namespace std;

namespace movit {

class Input;

namespace {

// Not thread-safe, but this isn't a big problem for testing.
ResourcePool *get_static_pool()
{
	static ResourcePool *resource_pool = NULL;
	if (!resource_pool) {
		resource_pool = new ResourcePool();
	}
	return resource_pool;
}

// Flip upside-down to compensate for different origin.
template<class T>
void vertical_flip(T *data, unsigned width, unsigned height)
{
	for (unsigned y = 0; y < height / 2; ++y) {
		unsigned flip_y = height - y - 1;
		for (unsigned x = 0; x < width; ++x) {
			swap(data[y * width + x], data[flip_y * width + x]);
		}
	}
}

}  // namespace

EffectChainTester::EffectChainTester(const float *data, unsigned width, unsigned height,
                                     MovitPixelFormat pixel_format, Colorspace color_space, GammaCurve gamma_curve,
                                     GLenum framebuffer_format)
	: chain(width, height, get_static_pool()),
	  width(width),
	  height(height),
	  framebuffer_format(framebuffer_format),
	  output_added(false),
	  finalized(false)
{
	CHECK(init_movit(".", MOVIT_DEBUG_OFF));

	if (data != NULL) {
		add_input(data, pixel_format, color_space, gamma_curve);
	}
}

EffectChainTester::~EffectChainTester()
{
}

Input *EffectChainTester::add_input(const float *data, MovitPixelFormat pixel_format, Colorspace color_space, GammaCurve gamma_curve, int input_width, int input_height)
{
	ImageFormat format;
	format.color_space = color_space;
	format.gamma_curve = gamma_curve;

	if (input_width == -1) {
		input_width = width;
	}
	if (input_height == -1) {
		input_height = height;
	}

	FlatInput *input = new FlatInput(format, pixel_format, GL_FLOAT, input_width, input_height);
	input->set_pixel_data(data);
	chain.add_input(input);
	return input;
}

Input *EffectChainTester::add_input(const unsigned char *data, MovitPixelFormat pixel_format, Colorspace color_space, GammaCurve gamma_curve, int input_width, int input_height)
{
	ImageFormat format;
	format.color_space = color_space;
	format.gamma_curve = gamma_curve;

	if (input_width == -1) {
		input_width = width;
	}
	if (input_height == -1) {
		input_height = height;
	}

	FlatInput *input = new FlatInput(format, pixel_format, GL_UNSIGNED_BYTE, input_width, input_height);
	input->set_pixel_data(data);
	chain.add_input(input);
	return input;
}

void EffectChainTester::run(float *out_data, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run<float>(out_data, NULL, NULL, NULL, GL_FLOAT, format, color_space, gamma_curve, alpha_format);
}

void EffectChainTester::run(float *out_data, float *out_data2, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run<float>(out_data, out_data2, NULL, NULL, GL_FLOAT, format, color_space, gamma_curve, alpha_format);
}

void EffectChainTester::run(float *out_data, float *out_data2, float *out_data3, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run<float>(out_data, out_data2, out_data3, NULL, GL_FLOAT, format, color_space, gamma_curve, alpha_format);
}

void EffectChainTester::run(float *out_data, float *out_data2, float *out_data3, float *out_data4, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run(out_data, out_data2, out_data3, out_data4, GL_FLOAT, format, color_space, gamma_curve, alpha_format);
}

void EffectChainTester::run(unsigned char *out_data, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run<unsigned char>(out_data, NULL, NULL, NULL, GL_UNSIGNED_BYTE, format, color_space, gamma_curve, alpha_format);
}

void EffectChainTester::run(unsigned char *out_data, unsigned char *out_data2, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run<unsigned char>(out_data, out_data2, NULL, NULL, GL_UNSIGNED_BYTE, format, color_space, gamma_curve, alpha_format);
}

void EffectChainTester::run(unsigned char *out_data, unsigned char *out_data2, unsigned char *out_data3, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run<unsigned char>(out_data, out_data2, out_data3, NULL, GL_UNSIGNED_BYTE, format, color_space, gamma_curve, alpha_format);
}

void EffectChainTester::run(unsigned char *out_data, unsigned char *out_data2, unsigned char *out_data3, unsigned char *out_data4, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run(out_data, out_data2, out_data3, out_data4, GL_UNSIGNED_BYTE, format, color_space, gamma_curve, alpha_format);
}

void EffectChainTester::run(uint16_t *out_data, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run<uint16_t>(out_data, NULL, NULL, NULL, GL_UNSIGNED_SHORT, format, color_space, gamma_curve, alpha_format);
}

void EffectChainTester::run_10_10_10_2(uint32_t *out_data, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	internal_run<uint32_t>(out_data, NULL, NULL, NULL, GL_UNSIGNED_INT_2_10_10_10_REV, format, color_space, gamma_curve, alpha_format);
}

template<class T>
void EffectChainTester::internal_run(T *out_data, T *out_data2, T *out_data3, T *out_data4, GLenum internal_format, GLenum format, Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	if (!finalized) {
		finalize_chain(color_space, gamma_curve, alpha_format);
	}

	GLuint type;
	if (framebuffer_format == GL_RGBA8) {
		type = GL_UNSIGNED_BYTE;
	} else if (framebuffer_format == GL_RGBA16) {
		type = GL_UNSIGNED_SHORT;
	} else if (framebuffer_format == GL_RGBA16F || framebuffer_format == GL_RGBA32F) {
		type = GL_FLOAT;
	} else if (framebuffer_format == GL_RGB10_A2) {
		type = GL_UNSIGNED_INT_2_10_10_10_REV;
	} else {
		// Add more here as needed.
		assert(false);
	}

	unsigned num_outputs;
	if (out_data4 != NULL) {
		num_outputs = 4;
	} else if (out_data3 != NULL) {
		num_outputs = 3;
	} else if (out_data2 != NULL) {
		num_outputs = 2;
	} else {
		num_outputs = 1;
	}

	GLuint fbo, texnum[4];

	glGenTextures(num_outputs, texnum);
	check_error();
	for (unsigned i = 0; i < num_outputs; ++i) {
		glBindTexture(GL_TEXTURE_2D, texnum[i]);
		check_error();
		glTexImage2D(GL_TEXTURE_2D, 0, framebuffer_format, width, height, 0, GL_RGBA, type, NULL);
		check_error();
	}

	glGenFramebuffers(1, &fbo);
	check_error();
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	check_error();
	for (unsigned i = 0; i < num_outputs; ++i) {
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_COLOR_ATTACHMENT0 + i,
			GL_TEXTURE_2D,
			texnum[i],
			0);
		check_error();
	}

	GLenum bufs[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(num_outputs, bufs);

	chain.render_to_fbo(fbo, width, height);

	T *data[4] = { out_data, out_data2, out_data3, out_data4 };

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	check_error();
	for (unsigned i = 0; i < num_outputs; ++i) {
		T *ptr = data[i];
		glReadBuffer(GL_COLOR_ATTACHMENT0 + i);
		if (!epoxy_is_desktop_gl() && (format == GL_RED || format == GL_BLUE || format == GL_ALPHA)) {
			// GLES will only read GL_RGBA.
			T *temp = new T[width * height * 4];
			glReadPixels(0, 0, width, height, GL_RGBA, internal_format, temp);
			check_error();
			if (format == GL_ALPHA) {
				for (unsigned i = 0; i < width * height; ++i) {
					ptr[i] = temp[i * 4 + 3];
				}
			} else if (format == GL_BLUE) {
				for (unsigned i = 0; i < width * height; ++i) {
					ptr[i] = temp[i * 4 + 2];
				}
			} else {
				for (unsigned i = 0; i < width * height; ++i) {
					ptr[i] = temp[i * 4];
				}
			}
			delete[] temp;
		} else {
			glReadPixels(0, 0, width, height, format, internal_format, ptr);
			check_error();
		}

		if (format == GL_RGBA && (type == GL_UNSIGNED_BYTE || type == GL_UNSIGNED_SHORT || type == GL_FLOAT)) {
			vertical_flip(ptr, width * 4, height);
		} else {
			vertical_flip(ptr, width, height);
		}
	}

	glDeleteFramebuffers(1, &fbo);
	check_error();
	glDeleteTextures(num_outputs, texnum);
	check_error();
}

void EffectChainTester::add_output(const ImageFormat &format, OutputAlphaFormat alpha_format)
{
	chain.add_output(format, alpha_format);
	output_added = true;
}

void EffectChainTester::add_ycbcr_output(const ImageFormat &format, OutputAlphaFormat alpha_format, const YCbCrFormat &ycbcr_format, YCbCrOutputSplitting output_splitting, GLenum type)
{
	chain.add_ycbcr_output(format, alpha_format, ycbcr_format, output_splitting, type);
	output_added = true;
}

void EffectChainTester::finalize_chain(Colorspace color_space, GammaCurve gamma_curve, OutputAlphaFormat alpha_format)
{
	assert(!finalized);
	if (!output_added) {
		ImageFormat image_format;
		image_format.color_space = color_space;
		image_format.gamma_curve = gamma_curve;
		chain.add_output(image_format, alpha_format);
		output_added = true;
	}
	chain.finalize();
	finalized = true;
}

void expect_equal(const float *ref, const float *result, unsigned width, unsigned height, float largest_difference_limit, float rms_limit)
{
	float largest_difference = -1.0f;
	float squared_difference = 0.0f;
	int largest_diff_x = -1, largest_diff_y = -1;

	for (unsigned y = 0; y < height; ++y) {
		for (unsigned x = 0; x < width; ++x) {
			float diff = ref[y * width + x] - result[y * width + x];
			if (fabs(diff) > largest_difference) {
				largest_difference = fabs(diff);
				largest_diff_x = x;
				largest_diff_y = y;
			}
			squared_difference += diff * diff;
		}
	}

	EXPECT_LT(largest_difference, largest_difference_limit)
		<< "Largest difference is in x=" << largest_diff_x << ", y=" << largest_diff_y << ":\n"
		<< "Reference: " << ref[largest_diff_y * width + largest_diff_x] << "\n"
		<< "Result:    " << result[largest_diff_y * width + largest_diff_x];

	float rms = sqrt(squared_difference) / (width * height);
	EXPECT_LT(rms, rms_limit);

	if (largest_difference >= largest_difference_limit || rms >= rms_limit) {
		fprintf(stderr, "Dumping matrices for easier debugging, since at least one test failed.\n");

		fprintf(stderr, "Reference:\n");
		for (unsigned y = 0; y < height; ++y) {
			for (unsigned x = 0; x < width; ++x) {
				fprintf(stderr, "%7.4f ", ref[y * width + x]);
			}
			fprintf(stderr, "\n");
		}

		fprintf(stderr, "\nResult:\n");
		for (unsigned y = 0; y < height; ++y) {
			for (unsigned x = 0; x < width; ++x) {
				fprintf(stderr, "%7.4f ", result[y * width + x]);
			}
			fprintf(stderr, "\n");
		}
	}
}

void expect_equal(const unsigned char *ref, const unsigned char *result, unsigned width, unsigned height, unsigned largest_difference_limit, float rms_limit)
{
	assert(width > 0);
	assert(height > 0);

	float *ref_float = new float[width * height];
	float *result_float = new float[width * height];

	for (unsigned y = 0; y < height; ++y) {
		for (unsigned x = 0; x < width; ++x) {
			ref_float[y * width + x] = ref[y * width + x];
			result_float[y * width + x] = result[y * width + x];
		}
	}

	expect_equal(ref_float, result_float, width, height, largest_difference_limit, rms_limit);

	delete[] ref_float;
	delete[] result_float;
}

void expect_equal(const uint16_t *ref, const uint16_t *result, unsigned width, unsigned height, unsigned largest_difference_limit, float rms_limit)
{
	assert(width > 0);
	assert(height > 0);

	float *ref_float = new float[width * height];
	float *result_float = new float[width * height];

	for (unsigned y = 0; y < height; ++y) {
		for (unsigned x = 0; x < width; ++x) {
			ref_float[y * width + x] = ref[y * width + x];
			result_float[y * width + x] = result[y * width + x];
		}
	}

	expect_equal(ref_float, result_float, width, height, largest_difference_limit, rms_limit);

	delete[] ref_float;
	delete[] result_float;
}

void expect_equal(const int *ref, const int *result, unsigned width, unsigned height, unsigned largest_difference_limit, float rms_limit)
{
	assert(width > 0);
	assert(height > 0);

	float *ref_float = new float[width * height];
	float *result_float = new float[width * height];

	for (unsigned y = 0; y < height; ++y) {
		for (unsigned x = 0; x < width; ++x) {
			ref_float[y * width + x] = ref[y * width + x];
			result_float[y * width + x] = result[y * width + x];
		}
	}

	expect_equal(ref_float, result_float, width, height, largest_difference_limit, rms_limit);

	delete[] ref_float;
	delete[] result_float;
}

void test_accuracy(const float *expected, const float *result, unsigned num_values, double absolute_error_limit, double relative_error_limit, double local_relative_error_limit, double rms_limit)
{
	double squared_difference = 0.0;
	for (unsigned i = 0; i < num_values; ++i) {
		double absolute_error = fabs(expected[i] - result[i]);
		squared_difference += absolute_error * absolute_error;
		EXPECT_LT(absolute_error, absolute_error_limit);

		if (expected[i] > 0.0) {
			double relative_error = fabs(absolute_error / expected[i]);

			EXPECT_LT(relative_error, relative_error_limit);
		}
		if (i < num_values - 1) {
			double delta = expected[i + 1] - expected[i];
			double local_relative_error = fabs(absolute_error / delta);
			EXPECT_LT(local_relative_error, local_relative_error_limit);
		}
	}
	double rms = sqrt(squared_difference) / num_values;
	EXPECT_LT(rms, rms_limit);
}

double srgb_to_linear(double x)
{
	// From the Wikipedia article on sRGB.
	if (x < 0.04045) {
		return x / 12.92;
	} else {
		return pow((x + 0.055) / 1.055, 2.4);
	}
}

double linear_to_srgb(double x)
{
	// From the Wikipedia article on sRGB.
	if (x < 0.0031308) {
		return 12.92 * x;
	} else {
		return 1.055 * pow(x, 1.0 / 2.4) - 0.055;
	}
}

}  // namespace movit
