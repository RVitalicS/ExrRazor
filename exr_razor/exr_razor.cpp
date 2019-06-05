#define _CRT_SECURE_NO_WARNINGS

#include <string>
#include <regex>
#include <vector>
#include <iostream>

#include <OpenEXR/ImfTestFile.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfArray.h>




bool has_tag(std::string name);


std::string cuttag(std::string name);




// possible base names to search in channel name
std::vector <std::string> valid_names = {

    "Ci", "primary",

    "directDiffuseLobe", "indirectDiffuseLobe",

    "directSpecularPrimaryLobe", "indirectSpecularPrimaryLobe",
    "directSpecularRoughLobe", "indirectSpecularRoughLobe",
    "directSpecularClearcoatLobe", "indirectSpecularClearcoatLobe",
    "directSpecularIridescenceLobe", "indirectSpecularIridescenceLobe",

    "directSpecularFuzzLobe", "indirectSpecularFuzzLobe",

    "transmissiveGlassLobe",
    "directSpecularGlassLobe", "indirectSpecularGlassLobe",

    "subsurfaceLobe",
    "transmissiveSingleScatterLobe",

    "directDiffuse", "indirectDiffuse", "direct_diffuse", "indirect_diffuse",
    "diffuseDirect", "diffuseIndirect", "diffuse_direct", "diffuse_indirect",

    "directSpecular", "indirectSpecular", "direct_specular", "indirect_specular",
    "specularDirect", "specularIndirect", "specular_direct", "specular_indirect",

    "sheenDirect", "sheenIndirect", "sheen_direct", "sheen_indirect",
    "directSheen", "indirectSheen", "direct_sheen", "indirect_sheen",

    "coatDirect", "coatIndirect", "coat_direct", "coat_indirect",
    "directCoat", "indirectCoat", "direct_coat", "indirect_coat",

    "transmissionDirect", "transmissionIndirect", "transmission_direct", "transmission_indirect",
    "directTransmission", "indirectTransmission", "direct_transmission", "indirect_transmission",
    "transmissive",

    "sssDirect", "sssIndirect", "sss_direct", "sss_indirect",
    "directSss", "indirectSss", "direct_sss", "indirect_sss",
    "subsurface",

    "volumeDirect", "volumeIndirect", "volume_direct", "volume_indirect",
    "directVolume", "indirectVolume", "direct_volume", "indirect_volume",

    "emissive", "emission"
};




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
                        

            // create iterator for channels
            const Imf::ChannelList &channels = exr_inp.header().channels();
            

            // get number of channels
            // and check if file has LightGroup tags
            int channels_count = 0;
            bool has_tags = false;
            for (Imf::ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
            {   
                if (has_tags == false) { has_tags = has_tag(i.name()); }
                channels_count++;
            }
            

            // work with only files that have LightGroup tag
            if (has_tags)
            {

                // get resolution
                Imath::Box2i data_window = exr_inp.header().dataWindow();
                int width = data_window.max.x - data_window.min.x + 1;
                int height = data_window.max.y - data_window.min.y + 1;
            


                // get FrameBuffer object from input exr file
                Imf::FrameBuffer frameBuffer_inp;

                Imf::Array2D<float> **data_array = new Imf::Array2D<float>* [channels_count];
                for (int i = 0; i < channels_count; i++) { data_array[i] = new Imf::Array2D<float>; }
                
                int array_item = 0;
                for (Imf::ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
                {   
                    data_array[array_item]->resizeErase(height, width);
                    
                    frameBuffer_inp.insert(i.name(),
                                           Imf::Slice(i.channel().type,
                                                     (char *)(data_array[array_item][0][0] -
                                                               data_window.min.x -
                                                               data_window.min.y * width
                                                             ),
                                                      sizeof(*data_array[array_item][0][0]) * 1,
                                                      sizeof(*data_array[array_item][0][0]) * width,
                                                      1, 1,
                                                      0.0
                                                     )
                                          );

                    array_item++;
                }

                exr_inp.setFrameBuffer(frameBuffer_inp);
                exr_inp.readPixels(data_window.min.y, data_window.max.y);
                


                // create new Header and FrameBuffer objects for output exr file
                Imf::FrameBuffer frameBuffer_out;
                Imf::Header header_out;

                
                // copy all attributes from input Header to output Header except "channels" data
                const Imf::Header &header_inp = exr_inp.header();
                for (Imf::Header::ConstIterator i = header_inp.begin(); i != header_inp.end(); ++i)
                {
                    if ((std::string)i.name() != (std::string)"channels")
                    {   

                        header_out.erase(i.name());
                        header_out.insert(i.name(), header_inp[i.name()]);
                    }
                }
                

                // for each channel in input file
                for (Imf::ChannelList::ConstIterator i = channels.begin(); i != channels.end(); ++i)
                {
                    // read channel name
                    const char *channel_name = i.name();


                    // create switch to do not include empty 'rgba' pass
                    bool include_channel = true;
                    std::vector <std::string> invalid_names = { "R", "G", "B", "A", "r", "g", "b", "a" };

                    for (int ii = 0; ii < invalid_names.size(); ii++)
                    {
                        if ((std::string)channel_name == invalid_names[ii])
                        {
                            for (Imf::ChannelList::ConstIterator iii = channels.begin(); iii != channels.end(); ++iii)
                            {
                                if ( channel_name != iii.name() )
                                {   
                                    // do not include channel if there is one with the same name after "cuttag"
                                    if ((std::string)channel_name == cuttag(iii.name())) { include_channel = false; }
                                }
                            }
                        }
                    }


                    // insert channels
                    if (include_channel)
                    {
                        // get edited name
                        std::string channel_name_edit = cuttag(channel_name);
                        
                        
                        // add output Header item with edited name
                        header_out.channels().insert(channel_name_edit, i.channel());
                        
                        
                        // add output FrameBuffer item
                        frameBuffer_out.insert(channel_name_edit, exr_inp.frameBuffer()[i.name()]);

                    }

                }

                
                // overwrite input file with changed data
                Imf::OutputFile exr_out(argv[i], header_out);
                exr_out.setFrameBuffer(frameBuffer_out);
                exr_out.writePixels(data_window.max.y - data_window.min.y + 1);

                
                // deallocate memory
                for (int i = 0; i < channels_count; i++) { data_array[i]->~Array2D(); }
                delete[] data_array;


                // inform of this action
                std::cout << "[INFO exr_razor.exe]: " << argv[i] << " - lightGroup tags has been cutted" << std::endl;
            }

            
            // inform of this action
            else { std::cout << "[INFO exr_razor.exe]: " << argv[i] << " - file has not been processed" << std::endl; }
        }
    }
    
    return 0;
}




/**
    Check if channel name has LightGroup tag

    @param name     channel name to check
    @return         result of comparison
*/
bool has_tag(std::string name)
{
    if (name != cuttag(name))
    { return true; }
    else 
    { return false; }
}



/**
    Replaces tagged name with standard for compositing channel name

    @param name     channel name to change
    @return         formatted string
*/
std::string cuttag(std::string name)
{   

    for (int i = 0; i < valid_names.size(); i++)
    {
        
        // create expression string
        std::stringstream regex_stream;
        regex_stream << valid_names[i].data() << ".*\\.";
        std::string regex_string = regex_stream.str();


        // create replacing string
        std::stringstream output_stream;
        output_stream << valid_names[i].data() << ".";
        std::string output_string = output_stream.str();


        // define exceptions
        if (valid_names[i] == (std::string)"Ci") { output_string = ""; }
        if (valid_names[i] == (std::string)"primary") { output_string = ""; }


        // replace input name with valid one
        name = std::regex_replace(name, std::regex(regex_string), output_string);
    }
    
    return name;
}
