#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <regex>

#include <OpenEXR/ImfTestFile.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfArray.h>



void read_exr_channel(Imf::InputFile &file_inp,
					  const Imath::Box2i &data_window,
					  const int &width,
					  const int &height,
					  const char channel_name[],
					  Imf::Array2D<float> &pixels_array);

std::string cuttag(std::string name);



/** RENAMES CHANNELS FOR IDENTICAL READING WITH TEMPLATE STRUCTURE OF COMPOSITING SOFTWARE */
int main(int argc, char *argv[])
{	

	// for each input exr file as argument
	for (int i = 1; i < argc; ++i)
	{
		if (Imf::isOpenExrFile(argv[i]))
		{

			// read exr file and load its data
			Imf::InputFile exr_inp(argv[i]);

			Imath::Box2i data_window = exr_inp.header().dataWindow();
			int width = data_window.max.x - data_window.min.x + 1;
			int height = data_window.max.y - data_window.min.y + 1;


			// create new Header and FrameBuffer objects for output exr file
			Imf::Header header_out(width, height);
			Imf::FrameBuffer frameBuffer_out;


			// copy all attributes from input Header to output Header except "channels" data
			const Imf::Header &header_inp = exr_inp.header();
			for (Imf::Header::ConstIterator i = header_inp.begin(); i != header_inp.end(); ++i)
			{
				const char *attribute_name = i.name();
				if ((std::string)attribute_name != (std::string)"channels")
				{
					header_out.insert(i.name(), i.attribute());
				}
			}


			// create iterator for channels in input Header
			const Imf::ChannelList &channels = exr_inp.header().channels();


			// get number of channels of input exr file
			int channels_count = 0;
			for (Imf::ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i) { channels_count++; }


			// allocate memory for dynamic array of pointers to dynamic arrays of floats to store channel data
			float **data_list = new float*[channels_count];
			for (int i = 0; i < channels_count; i++) { data_list[i] = new float[width*height]; }


			// for each channel in input file
			int array_item = 0;
			for (Imf::ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
			{

				// read channel name and change it if it necessary
				const char *channel_name = i.name();
				std::string channel_name_edit = cuttag(channel_name);


				// add output Header item with edited name
				header_out.channels().insert(channel_name_edit, i.channel());


				// read channel data from file and convert those data to one level float array 
				Imf::Array2D<float> pixels_data;
				read_exr_channel(exr_inp, data_window, width, height, channel_name, pixels_data);

				for (int h = 0; h < (height); h++) {
					for (int w = 0; w < (width); w++) {

						int i = h * width + w;
						data_list[array_item][i] = pixels_data[h][w];
					}
				}


				// add output FrameBuffer item of pixel data for current channel
				frameBuffer_out.insert(channel_name_edit,
									   Imf::Slice(Imf::FLOAT,
												  (char *)data_list[array_item],
												  sizeof(*data_list[array_item]) * 1,
												  sizeof(*data_list[array_item]) * width
												 )
									  );

				array_item++;
			}


			// overwrite input file with changed data
			Imf::OutputFile exr_out(argv[i], header_out);
			exr_out.setFrameBuffer(frameBuffer_out);
			exr_out.writePixels(height);


			// deallocate memory
			for (int i = 0; i < channels_count; i++) { delete[] data_list[i]; }
			delete[] data_list;
		}
	}

	return 0;
}


/**
	Replaces tagged name with standard for compositing channel name

	@param name string with or without LightGroup tag
	@return formatted string
*/
std::string cuttag(std::string name)
{
	name = std::regex_replace(name, std::regex("Ci.*\\."), "");
	name = std::regex_replace(name, std::regex("directDiffuse.*\\."), "directDiffuse.");
	name = std::regex_replace(name, std::regex("directSpecular.*\\."), "directSpecular.");
	name = std::regex_replace(name, std::regex("indirectDiffuse.*\\."), "indirectDiffuse.");
	name = std::regex_replace(name, std::regex("indirectSpecular.*\\."), "indirectSpecular.");
	name = std::regex_replace(name, std::regex("transmissive.*\\."), "transmissive.");
	name = std::regex_replace(name, std::regex("subsurface.*\\."), "subsurface.");
	name = std::regex_replace(name, std::regex("emissive.*\\."), "emissive.");

	return name;
}


/**
	Reads named channel data from exr file and fills external array

	@param file_inp      name of the file to read data
	@param data_window   the region of buffer for which valid pixel data exist
	@param width         number of pixels of the width of target channel
	@param height        number of pixels of the height of target channel
	@param channel_name  name of target channel to read data
	@param pixels_array  external array to fill with channel data
*/
void read_exr_channel(Imf::InputFile &file_inp,
					  const Imath::Box2i &data_window,
					  const int &width,
					  const int &height,
					  const char channel_name[],
					  Imf::Array2D<float> &pixels_array)
{
	pixels_array.resizeErase(height, width);

	Imf::FrameBuffer frameBuffer;

	frameBuffer.insert(channel_name,
					   Imf::Slice(Imf::FLOAT,
								  (char *) (&pixels_array[0][0] -
											data_window.min.x -
											data_window.min.y * width
										   ),
								  sizeof(pixels_array[0][0]) * 1,
								  sizeof(pixels_array[0][0]) * width,
								  1, 1,
								  FLT_MAX
								 )
					  );

	file_inp.setFrameBuffer(frameBuffer);
	file_inp.readPixels(data_window.min.y, data_window.max.y);
}
