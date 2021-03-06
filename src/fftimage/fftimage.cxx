/*=========================================================================
*
* Copyright Marius Staring, Stefan Klein, David Doria. 2011.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0.txt
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*=========================================================================*/
/** \file
 \brief Compute the FFT of an image. Note this documentation may be empty if ITKTools was compiled without USE_FFTIMAGE.

 \verbinclude fftimage.help
 */
#include "itkUseMevisDicomTiff.h"
#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"
#include <itksys/SystemTools.hxx>
#include "ITKToolsImageProperties.h"
#include "ITKToolsHelpers.h"

//#include "itkFFTWRealToComplexConjugateImageFilter.h"
#include "itkFFTWForwardFFTImageFilter.h"
//#include "itkFFTWComplexConjugateToRealImageFilter.h"
#include "itkFFTWInverseFFTImageFilter.h"
#include "itkComplexToRealImageFilter.h"
#include "itkComplexToImaginaryImageFilter.h"
//#include "itkComposeComplexImageFilter.h"
#include "itkComposeImageFilter.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

//-------------------------------------------------------------------------------------


/**
  * ******************* GetHelpString *******************
  */
std::string GetHelpString( void )
{
  std::stringstream ss;
  ss << "ITKTools v" << itktools::GetITKToolsVersion() << "\n"
  << "Usage:" << std::endl
  << "pxfftimage" << std::endl
  << "  -in      inputFilenames" << std::endl
  << "             forward: only one input" << std::endl
  << "             backward, # given:" << std::endl
  << "               1: a complex image" << std::endl
  << "               2: a real and imaginary part" << std::endl
  << "  -op      operator, {forward, backward} FFT" << std::endl
  << "  [-out]   outputFilenames" << std::endl
  << "             forward, # given:" << std::endl
  << "               1: write the complex image, default in + Complex.mhd" << std::endl
  << "               2: write the real and imaginary images, default in + Real.mhd and in + Imaginary.mhd" << std::endl
  << "               3: write the complex, real and imaginary images" << std::endl
  << "             backward: only one output, default in + IFFT" << std::endl
  << "  [-opct]  the output type" << std::endl
  << "             choose from {float, double}, default float" << std::endl
//  << "  [-xdim]  the backward transform needs to know if the actual x-dimension was odd or even." << std::endl
//  << "             choose from {odd, even}, default even" << std::endl
  << "Supported: 2D, 3D, (unsigned) char, (unsigned) short, (unsigned) int, (unsigned) long, float, double.";

  return ss.str();

} // end GetHelpString()


/** run: A macro to call a function. */
#define run( type, dim ) \
if ( componentType == #type && Dimension == dim ) \
{ \
  if ( op == "forward" ) \
  { \
    FFTImage< type, dim >( inputFileNames[ 0 ], outputFileNames ); \
  } \
  else \
  { \
    IFFTImage< type, dim >( inputFileNames, outputFileNames[ 0 ] ); \
  } \
}

//-------------------------------------------------------------------------------------

/* Declare FFTImage. */
template< class PixelType, unsigned int Dimension >
void FFTImage( const std::string & inputFileName,
  const std::vector<std::string> & outputFileNames );

/* Declare IFFTImage. */
template< class PixelType, unsigned int Dimension >
void IFFTImage( const std::vector<std::string> & inputFileNames,
  const std::string & outputFileName );

/** Declare other functions. */
std::string GetHelpString( void );

//-------------------------------------------------------------------------------------

int main( int argc, char **argv )
{
  RegisterMevisDicomTiff();

  /** Create a command line argument parser. */
  itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
  parser->SetCommandLineArguments( argc, argv );
  parser->SetProgramHelpText(GetHelpString());

  parser->MarkArgumentAsRequired( "-in", "The input filename." );
  parser->MarkArgumentAsRequired( "-op", "The operation to perform." );

  itk::CommandLineArgumentParser::ReturnValue validateArguments = parser->CheckForRequiredArguments();

  if(validateArguments == itk::CommandLineArgumentParser::FAILED)
  {
    return EXIT_FAILURE;
  }
  else if(validateArguments == itk::CommandLineArgumentParser::HELPREQUESTED)
  {
    return EXIT_SUCCESS;
  }

  /** Get arguments. */
  std::vector<std::string>  inputFileNames;
  parser->GetCommandLineArgument( "-in", inputFileNames );

  std::vector<std::string>  outputFileNames;
  bool retout = parser->GetCommandLineArgument( "-out", outputFileNames );

  std::string op = "";
  parser->GetCommandLineArgument( "-op", op );

  std::string componentType = "float";
  bool retopct = parser->GetCommandLineArgument( "-opct", componentType );

  //std::string xdim = "even";
  //bool retxdim = parser->GetCommandLineArgument( "-xdim", xdim );

  /** Check operator. */
  op = itksys::SystemTools::LowerCase( op );
  if ( op == "inverse" ) op = "backward";
  if ( op != "forward" && op != "backward" )
  {
    std::cerr << "ERROR: \"-op\" should be one of {forward, backward}." << std::endl;
    return 1;
  }

  /** Check input. */
  if ( op == "forward" && inputFileNames.size() > 1 )
  {
    std::cerr << "ERROR: Only one input file is expected." << std::endl;
    return 1;
  }
  if ( op == "backward" && inputFileNames.size() > 2 )
  {
    std::cerr << "ERROR: Only one or two input files are expected." << std::endl;
    return 1;
  }

  /** Check xdim. 
  if ( op == "backward" && retxdim )
  {
    if ( xdim != "odd" && xdim != "even" )
    {
      std::cerr << "ERROR: \"-xdim\" should be one of {odd, even}." << std::endl;
      return 1;
    }
  }
  */

  /** Check output names. */
  if ( outputFileNames.size() == 0 )
  {
    std::string inputpart = inputFileNames[ 0 ].substr( 0, inputFileNames[ 0 ].rfind( "." ) );
    if ( op == "forward" )
    {
    outputFileNames.resize( 3 );
      outputFileNames[ 0 ] = inputpart + "Complex.mhd";
      outputFileNames[ 1 ] = inputpart + "Real.mhd";
      outputFileNames[ 2 ] = inputpart + "Imaginary.mhd";
    }
    else if ( op == "backward" )
    {
    outputFileNames.resize( 1 );
      outputFileNames[ 0 ] = inputpart + "IFFT.mhd";
    }
  }

  /** Determine image properties. */
  std::string ComponentTypeIn = "short";
  std::string PixelType; //we don't use this
  unsigned int Dimension = 3;
  unsigned int NumberOfComponents = 1;
  std::vector<unsigned int> imagesize( Dimension, 0 );
  int retgip = itktools::GetImageProperties(
    inputFileNames[ 0 ],
    PixelType,
    ComponentTypeIn,
    Dimension,
    NumberOfComponents,
    imagesize );
  if ( retgip != 0 )
  {
    return 1;
  }

  /** Check for vector images. */
  if ( op == "forward" && NumberOfComponents > 1 )
  {
    std::cerr << "ERROR: The NumberOfComponents is larger than 1!" << std::endl;
    std::cerr << "Cannot take the forward Fourier transform of vector images." << std::endl;
    return 1;
  }
  if ( op == "backward" && inputFileNames.size() == 1 && NumberOfComponents != 2 )
  {
    std::cerr << "ERROR: The NumberOfComponents is not 2!" << std::endl;
    std::cerr << "Cannot take the inverse Fourier transform of non-complex images." << std::endl;
    return 1;
  }
  if ( op == "backward" && inputFileNames.size() == 2 && NumberOfComponents != 1 )
  {
    std::cerr << "ERROR: The NumberOfComponents is not 1!" << std::endl;
    std::cerr << "If two input images are given, they are expected to be two scalar images." << std::endl;
    std::cerr << "The first image is considered the real part, the second the imaginary part." << std::endl;
    return 1;
  }

  /** Run the program. */
  try
  {
    /** \todo: rewrite to new-style itktools */
    run( float, 2 );
    run( double, 2 );

    run( float, 3 );
    run( double, 3 );
  }
  catch( itk::ExceptionObject &e )
  {
    std::cerr << "Caught ITK exception: " << e << std::endl;
    return 1;
  }

  /** End program. */
  return 0;

} // end main()


  /*
   * ******************* FFTImage *******************
   */

template< class PixelType, unsigned int Dimension >
void FFTImage( const std::string & inputFileName,
  const std::vector<std::string> & outputFileNames )
{
  /** Typedefs. */
  typedef itk::Image< PixelType, Dimension >        ImageType;
  typedef itk::ImageFileReader< ImageType >         ReaderType;
  typedef itk::ImageFileWriter< ImageType >         WriterType;
  typedef itk::FFTWForwardFFTImageFilter< 
    ImageType >                                     FFTFilterType;
  typedef typename FFTFilterType::OutputImageType   ComplexImageType;
  typedef itk::ImageFileWriter< ComplexImageType >  ComplexWriterType;
  typedef itk::ComplexToRealImageFilter<
    ComplexImageType, ImageType >                   RealFilterType;
  typedef itk::ComplexToImaginaryImageFilter<
    ComplexImageType, ImageType >                   ImaginaryFilterType;

  /** Read the image as float or double. */
  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( inputFileName.c_str() );

  /** Compute the FFT of the image. */
  typename FFTFilterType::Pointer fftFilter = FFTFilterType::New();
  fftFilter->SetInput( reader->GetOutput() );
  fftFilter->Update();

  /** Write the output image(s).
   * If 1 output image is given: write the complex image.
   * If 2 output images are given: write the real and imaginary images.
   * If 3 output images are given: write the complex, real and imaginary images.
   */
  if ( outputFileNames.size() == 1 || outputFileNames.size() == 3 )
  {
    typename ComplexWriterType::Pointer complexWriter = ComplexWriterType::New();
    complexWriter->SetFileName( outputFileNames[ 0 ].c_str() );
    complexWriter->SetInput( fftFilter->GetOutput() );
    complexWriter->Update();
  }
  if ( outputFileNames.size() > 1 )
  {
    typename RealFilterType::Pointer realFilter = RealFilterType::New();
    realFilter->SetInput( fftFilter->GetOutput() );

    typename ImaginaryFilterType::Pointer imaginaryFilter = ImaginaryFilterType::New();
    imaginaryFilter->SetInput( fftFilter->GetOutput() );

    typename WriterType::Pointer writer1 = WriterType::New();
    typename WriterType::Pointer writer2 = WriterType::New();

    if ( outputFileNames.size() == 2 )
    {
      writer1->SetFileName( outputFileNames[ 0 ].c_str() );
      writer2->SetFileName( outputFileNames[ 1 ].c_str() );
    }
    else
    {
      writer1->SetFileName( outputFileNames[ 1 ].c_str() );
      writer2->SetFileName( outputFileNames[ 2 ].c_str() );
    }
    writer1->SetInput( realFilter->GetOutput() );
    writer1->Update();

    writer2->SetInput( imaginaryFilter->GetOutput() );
    writer2->Update();
  }

} // end FFTImage()


  /*
   * ******************* IFFTImage *******************
   */

template< class PixelType, unsigned int Dimension >
void IFFTImage( const std::vector<std::string> & inputFileNames,
  const std::string & outputFileName )
{
  /** Typedefs. */
  typedef itk::Image< PixelType, Dimension >            ImageType;
  typedef itk::Image< 
    std::complex< PixelType >, Dimension >                ComplexImageType;
  typedef itk::FFTWInverseFFTImageFilter<
    ComplexImageType >                                  IFFTFilterType;
  typedef itk::ImageFileReader< ImageType >             ReaderType;
  typedef itk::ImageFileReader< ComplexImageType >      ComplexReaderType;
  typedef itk::ComposeImageFilter<
    ImageType, ComplexImageType>                        ComposeComplexImageFilterType;
  typedef itk::ImageFileWriter< ImageType >             WriterType;

  /** The IFFT of the image. */
  typename IFFTFilterType::Pointer ifftFilter = IFFTFilterType::New();

  /** Read one complex image, or two scalar images, which need to
   * be combined into one complex image. */
  typename ComplexReaderType::Pointer complexReader
    = ComplexReaderType::New();
  typename ReaderType::Pointer reader1 = ReaderType::New();
  typename ReaderType::Pointer reader2 = ReaderType::New();
  typename ComposeComplexImageFilterType::Pointer composer
    = ComposeComplexImageFilterType::New();
  if ( inputFileNames.size() == 1 )
  {
    complexReader->SetFileName( inputFileNames[ 0 ].c_str() );
    ifftFilter->SetInput( complexReader->GetOutput() );
  }
  else if  ( inputFileNames.size() == 2 )
  {
    reader1->SetFileName( inputFileNames[ 0 ].c_str() );
    reader2->SetFileName( inputFileNames[ 1 ].c_str() );
    composer->SetInput( 0, reader1->GetOutput() );
    composer->SetInput( 1, reader2->GetOutput() );
    ifftFilter->SetInput( composer->GetOutput() );
  }

  /** Setup IFFT filter. 
  if ( xdim == "odd" )
  {
    ifftFilter->SetActualXDimensionIsOdd( true );
  }
  else
  {
    ifftFilter->SetActualXDimensionIsOdd( false );
  }
  */

  /** Write the output image. */
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputFileName.c_str() );
  writer->SetInput( ifftFilter->GetOutput() );
  writer->Update();

} // end IFFTImage()

