
#include "libheif/heif.h"
#include "libheif/heif_plugin.h"
#include "heif_encoder_openjpeg.h"

#include <openjpeg.h>
#include <string.h>

#include <vector>
#include <string>
using namespace std;

#define WRITE_JP2 0

static const int OPJ_PLUGIN_PRIORITY = 80;
static struct heif_error error_Ok = {heif_error_Ok, heif_suberror_Unspecified, "Success"};

struct encoder_struct_opj {

  heif_chroma chroma;

  // --- output

  std::vector<uint8_t> codestream; //contains encoded pixel data
  bool data_read = false;

  // --- parameters

  // std::vector<parameter> parameters;

  // void add_param(const parameter&);

  // void add_param(const std::string& name, int value);

  // void add_param(const std::string& name, bool value);

  // void add_param(const std::string& name, const std::string& value);

  // parameter get_param(const std::string& name) const;

  // std::string preset;
  // std::string tune;

  // int logLevel = X265_LOG_NONE;
};


const char* opj_plugin_name() {
    return "OpenJPEG JPEG2000 Encoder"; // Human-readable name of the plugin
}

void opj_init_plugin() {
  // Global plugin initialization (may be NULL)
}

void opj_cleanup_plugin() {
  // Global plugin cleanup (may be NULL).
  // Free data that was allocated in init_plugin()
}

struct heif_error opj_new_encoder(void** encoder_out) {

  struct encoder_struct_opj* encoder = new encoder_struct_opj();
  encoder->chroma = heif_chroma_interleaved_RGB; //default chroma

  *encoder_out = encoder;
  return error_Ok;
}

void opj_free_encoder(void* encoder_raw) {
  // Free the decoder context (heif_image can still be used after destruction)
  struct encoder_struct_opj* encoder = (struct encoder_struct_opj*) encoder_raw;
  delete encoder;
}

struct heif_error opj_set_parameter_quality(void* encoder, int quality) {
  return error_Ok;
}

struct heif_error opj_get_parameter_quality(void* encoder, int* quality) {
  return error_Ok;
}

struct heif_error opj_set_parameter_lossless(void* encoder, int lossless) {
  return error_Ok;
}

struct heif_error opj_get_parameter_lossless(void* encoder, int* lossless) {
  return error_Ok;
}

struct heif_error opj_set_parameter_logging_level(void* encoder, int logging) {
  return error_Ok;
}

struct heif_error opj_get_parameter_logging_level(void* encoder, int* logging) {
  return error_Ok;
}

const struct heif_encoder_parameter** opj_list_parameters(void* encoder) {
  return nullptr;
}

struct heif_error opj_set_parameter_integer(void* encoder, const char* name, int value) {
  return error_Ok;
}

struct heif_error opj_get_parameter_integer(void* encoder, const char* name, int* value) {
  return error_Ok;
}

struct heif_error opj_set_parameter_boolean(void* encoder, const char* name, int value) {
  return error_Ok;
}

struct heif_error opj_get_parameter_boolean(void* encoder, const char* name, int* value) {
  return error_Ok;
}

struct heif_error opj_set_parameter_string(void* encoder, const char* name, const char* value) {
  return error_Ok;
}

struct heif_error opj_get_parameter_string(void* encoder, const char* name, char* value, int value_size) {
  return error_Ok;
}

void opj_query_input_colorspace(enum heif_colorspace* inout_colorspace, enum heif_chroma* inout_chroma) {
  // Replace the input colorspace/chroma with the one that is supported by the encoder and that
  // comes as close to the input colorspace/chroma as possible.
}

static OPJ_SIZE_T global_variable = 0;

static OPJ_SIZE_T opj_write_from_buffer(void* source, OPJ_SIZE_T nb_bytes, void* dest) {
    printf("%s()  - nb_bytes: %ld = 0x%lx\n", __FUNCTION__, nb_bytes, nb_bytes);
    memcpy(dest, source, nb_bytes);
    global_variable += nb_bytes;
    return nb_bytes;
}

static void opj_close_from_buffer(void* p_user_data)
{
    FILE* p_file = (FILE*)p_user_data;
    fclose(p_file);
}

static opj_image_t *to_opj_image(const unsigned char *buf, int width, int height, 
                                int band_count, int sub_dx, int sub_dy) {

	opj_image_cmptparm_t component_params[4];
	memset(&component_params, 0, 4 * sizeof(opj_image_cmptparm_t));
	for (int comp = 0; comp < band_count; ++comp) {
    component_params[comp].prec = 8;
    component_params[comp].bpp = 8;
    component_params[comp].sgnd = 0;
    component_params[comp].dx = sub_dx;
    component_params[comp].dy = sub_dy;
    component_params[comp].w = width;
    component_params[comp].h = height;
   }

  OPJ_COLOR_SPACE colorspace = (band_count > 2 ? OPJ_CLRSPC_SRGB: OPJ_CLRSPC_GRAY);
	opj_image_t *image = opj_image_create(band_count, &component_params[0], colorspace);
	if (image == NULL) {
    // Failed to create image
	  return NULL;
  }

  // ???????
  image->x0 = 0;
  image->y0 = 0;
  image->x1 = (width - 1) * sub_dx + 1;
  image->y1 = (height - 1) * sub_dy + 1;

	int *r, *g, *b, *a;
	bool is_rgb;
	if (band_count == 3) {
    is_rgb = 1;
    r = image->comps[0].data;
    g = image->comps[1].data;
    b = image->comps[2].data;
  }
	else { //Grayscale
	  r = image->comps[0].data;
  }

	const unsigned char *cs = buf;
	unsigned int max = height * width;
	for (int i = 0; i < max; ++i) {
	  if (is_rgb) {
      *r++ = (int)*cs++; 
      *g++ = (int)*cs++; 
      *b++ = (int)*cs++;
      continue;
    }

    /* G */
  	*r++ = (int)*cs++;
   }

	return image;

} 


static heif_error generate_codestream(const uint8_t* data, struct encoder_struct_opj* encoder, int width, int height, int numcomps) {

	opj_cparameters_t parameters;
	opj_set_default_encoder_parameters(&parameters);
	int sub_dx = parameters.subsampling_dx;
	int sub_dy = parameters.subsampling_dy;
  #if 0
  //Add comment
	if (parameters.cp_comment == NULL) {
    char buf[80];
    #ifdef _WIN32
    sprintf_s(buf, 80, "Created by OpenJPEG version %s", opj_version());
    #else
    snprintf(buf, 80, "Created by OpenJPEG version %s", opj_version());
    #endif
    parameters.cp_comment = strdup(buf);
  }
  #endif

  if (parameters.tcp_numlayers == 0) {
    parameters.tcp_rates[0] = 0;
    parameters.tcp_numlayers++;
    parameters.cp_disto_alloc = 1;
  }


  //Create opj image
	opj_image_t *image = to_opj_image(data, width, height, numcomps, sub_dx, sub_dy);

	opj_codec_t* codec;

	parameters.tcp_mct = image->numcomps == 3 ? 1 : 0;

	OPJ_CODEC_FORMAT codec_format = OPJ_CODEC_J2K; // OPJ_CODEC_JP2;
	codec = opj_create_compress(codec_format);

	opj_setup_encoder(codec, &parameters, image);

  size_t size = width * height * numcomps;
  uint8_t* out_data = (uint8_t*) malloc(size);
  opj_stream_t* l_stream = 00;

  // l_stream = opj_stream_create(OPJ_J2K_STREAM_CHUNK_SIZE, WRITE_JP2);
  l_stream = opj_stream_create(size, WRITE_JP2);

  opj_stream_set_user_data(l_stream, out_data, opj_close_from_buffer);
  opj_stream_set_write_function(l_stream, (opj_stream_write_fn) opj_write_from_buffer);
  // opj_stream_set_user_data_length(l_stream, 0);

	opj_stream_t *stream = l_stream;
  heif_error error;
	if (stream == NULL) {
    error = { heif_error_Encoding_error,
              heif_suberror_Unspecified,
              "Failed to create opj_stream_t"};
    return error;
  }


	if (!opj_start_compress(codec, image, stream)) {
    error = { heif_error_Encoding_error,
              heif_suberror_Unspecified,
              "Failed opj_start_compress()"};
    return error;
  }

	if (!opj_encode(codec, stream)) {
    error = { heif_error_Encoding_error,
              heif_suberror_Unspecified,
              "Failed opj_encode()"};
    return error;
  }

	opj_end_compress(codec, stream);

  for (OPJ_SIZE_T i = 0; i < global_variable; i++) {
    encoder->codestream.push_back(out_data[i]);
  }

  return error_Ok;
}

struct heif_error opj_encode_image(void* encoder_raw, const struct heif_image* image, enum heif_image_input_class image_class) {
  
  struct encoder_struct_opj* encoder = (struct encoder_struct_opj*) encoder_raw;
  struct heif_error err;


  heif_chroma chroma = heif_image_get_chroma_format(image);
  heif_colorspace colorspace = heif_image_get_colorspace(image);
  uint32_t numcomps;
  heif_channel channel;
  if (chroma == heif_chroma_interleaved_RGB) {
    channel = heif_channel_interleaved;
    numcomps = 3;
  } 
  else {
    err = { heif_error_Unsupported_feature, 
            heif_suberror_Unsupported_data_version, 
            "Chroma not yet supported"};
    return err;
  }

  if (colorspace == heif_colorspace_RGB) {
    //
  }
  else {
    err = { heif_error_Unsupported_feature, 
            heif_suberror_Unsupported_data_version,
            "Colorspace not yet supported"};
    return err;
  }



  int stride = 0;
  const uint8_t* src_data = heif_image_get_plane_readonly(image, channel, &stride);
  unsigned int width = heif_image_get_primary_width(image);
  unsigned int height = heif_image_get_primary_height(image);
  encoder->codestream.clear(); //Fixes issue when encoding multiple images and old data persists.
  err = generate_codestream(src_data, encoder, width, height, numcomps);

  return err;
}

struct heif_error opj_get_compressed_data(void* encoder_raw, uint8_t** data, int* size, enum heif_encoded_data_type* type) {
  // Get a packet of decoded data. The data format depends on the codec.
  
  struct encoder_struct_opj* encoder = (struct encoder_struct_opj*) encoder_raw;

  if (encoder->data_read) {
    *size = 0;
    *data = nullptr;
  } 
  else {
    *size = (int) encoder->codestream.size();
    *data = encoder->codestream.data();
    encoder->data_read = true;
  }

  return error_Ok;
}

void opj_query_input_colorspace2(void* encoder, enum heif_colorspace* inout_colorspace, enum heif_chroma* inout_chroma) {
  //TODO
}

void opj_query_encoded_size(void* encoder, uint32_t input_width, uint32_t input_height, uint32_t* encoded_width, uint32_t* encoded_height) {
  // --- version 3 ---

  // The encoded image size may be different from the input frame size, e.g. because
  // of required rounding, or a required minimum size. Use this function to return
  // the encoded size for a given input image size.
  // You may set this to NULL if no padding is required for any image size.
}


static const struct heif_encoder_plugin encoder_plugin_openjpeg
    {
        /* plugin_api_version */ 3,
        /* compression_format */ heif_compression_JPEG2000,
        /* id_name */ "OpenJPEG",
        /* priority */ OPJ_PLUGIN_PRIORITY,
        /* supports_lossy_compression */ false,
        /* supports_lossless_compression */ true,
        /* get_plugin_name */ opj_plugin_name,
        /* init_plugin */ opj_init_plugin,
        /* cleanup_plugin */ opj_cleanup_plugin,
        /* new_encoder */ opj_new_encoder,
        /* free_encoder */ opj_free_encoder,
        /* set_parameter_quality */ opj_set_parameter_quality,
        /* get_parameter_quality */ opj_get_parameter_quality,
        /* set_parameter_lossless */ opj_set_parameter_lossless,
        /* get_parameter_lossless */ opj_get_parameter_lossless,
        /* set_parameter_logging_level */ opj_set_parameter_logging_level,
        /* get_parameter_logging_level */ opj_get_parameter_logging_level,
        /* list_parameters */ opj_list_parameters,
        /* set_parameter_integer */ opj_set_parameter_integer,
        /* get_parameter_integer */ opj_get_parameter_integer,
        /* set_parameter_boolean */ opj_set_parameter_boolean,
        /* get_parameter_boolean */ opj_get_parameter_boolean,
        /* set_parameter_string */ opj_set_parameter_string,
        /* get_parameter_string */ opj_get_parameter_string,
        /* query_input_colorspace */ opj_query_input_colorspace,
        /* encode_image */ opj_encode_image,
        /* get_compressed_data */ opj_get_compressed_data,
        /* query_input_colorspace (v2) */ opj_query_input_colorspace2,
        /* query_encoded_size (v3) */ opj_query_encoded_size
    };

const struct heif_encoder_plugin* get_encoder_plugin_openjpeg() {
    return &encoder_plugin_openjpeg;
}
