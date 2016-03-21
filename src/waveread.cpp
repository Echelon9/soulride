/*
    Copyright 2000, 2001, 2002, 2003 Slingshot Game Technology, Inc.

    This file is part of The Soul Ride Engine, see http://soulride.com

    The Soul Ride Engine is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    The Soul Ride Engine is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
    See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Foobar; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
// waveread.cpp	-thatcher 5/23/1998 Copyright Slingshot

// Some functions for reading .WAV files.  Code mostly stolen from MS DirectX samples.


#include "waveread.hpp"

//
//
//
namespace WaveRead {


int WaveOpenFile(
	char *pszFileName,                              // (IN)
	HMMIO *phmmioIn,                                // (OUT)
	WAVEFORMATEX **ppwfxInfo,                       // (OUT)
	MMCKINFO *pckInRIFF                             // (OUT)
			)
/* This function will open a wave input file and prepare it for reading,
 * so the data can be easily
 * read with WaveReadFile. Returns 0 if successful, the error code if not.
 *      pszFileName - Input filename to load.
 *      phmmioIn    - Pointer to handle which will be used
 *          for further mmio routines.
 *      ppwfxInfo   - Ptr to ptr to WaveFormatEx structure
 *          with all info about the file.                        
 *      
*/
{
	HMMIO           hmmioIn;
	MMCKINFO        ckIn;           // chunk info. for general use.
	PCMWAVEFORMAT   pcmWaveFormat;  // Temp PCM structure to load in.       
	WORD            cbExtraAlloc;   // Extra bytes for waveformatex 
	int             nError;         // Return value.


	// Initialization...
	*ppwfxInfo = NULL;
	nError = 0;
	hmmioIn = NULL;
	
	if ((hmmioIn = mmioOpen(pszFileName, NULL, MMIO_ALLOCBUF | MMIO_READ)) == NULL)
		{
		nError = ER_CANNOTOPEN;
		goto ERROR_READING_WAVE;
		}

	if ((nError = (int)mmioDescend(hmmioIn, pckInRIFF, NULL, 0)) != 0)
		{
		goto ERROR_READING_WAVE;
		}


	if ((pckInRIFF->ckid != FOURCC_RIFF) || (pckInRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E')))
		{
		nError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
		}
			
	/* Search the input file for for the 'fmt ' chunk.     */
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if ((nError = (int)mmioDescend(hmmioIn, &ckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
		{
		goto ERROR_READING_WAVE;                
		}
					
	/* Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
    * if there are extra parameters at the end, we'll ignore them */
    
    if (ckIn.cksize < (long) sizeof(PCMWAVEFORMAT))
		{
		nError = ER_NOTWAVEFILE;
		goto ERROR_READING_WAVE;
		}
															
	/* Read the 'fmt ' chunk into <pcmWaveFormat>.*/     
    if (mmioRead(hmmioIn, (HPSTR) &pcmWaveFormat, (long) sizeof(pcmWaveFormat)) != (long) sizeof(pcmWaveFormat))
		{
		nError = ER_CANNOTREAD;
		goto ERROR_READING_WAVE;
		}
							

	// Ok, allocate the waveformatex, but if its not pcm
	// format, read the next word, and thats how many extra
	// bytes to allocate.
	if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
		cbExtraAlloc = 0;                               
							
	else
		{
		// Read in length of extra bytes.
		if (mmioRead(hmmioIn, (LPSTR) &cbExtraAlloc,
			(long) sizeof(cbExtraAlloc)) != (long) sizeof(cbExtraAlloc))
			{
			nError = ER_CANNOTREAD;
			goto ERROR_READING_WAVE;
			}

		}
							
	// Ok, now allocate that waveformatex structure.
	if ((*ppwfxInfo = GlobalAlloc(GMEM_FIXED, sizeof(WAVEFORMATEX)+cbExtraAlloc)) == NULL)
		{
		nError = ER_MEM;
		goto ERROR_READING_WAVE;
		}

	// Copy the bytes from the pcm structure to the waveformatex structure
	memcpy(*ppwfxInfo, &pcmWaveFormat, sizeof(pcmWaveFormat));
	(*ppwfxInfo)->cbSize = cbExtraAlloc;

	// Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
	if (cbExtraAlloc != 0)
		{
		if (mmioRead(hmmioIn, (LPSTR) (((BYTE*)&((*ppwfxInfo)->cbSize))+sizeof(cbExtraAlloc)),
			(long) (cbExtraAlloc)) != (long) (cbExtraAlloc))
			{
			nError = ER_NOTWAVEFILE;
			goto ERROR_READING_WAVE;
			}
		}

	/* Ascend the input file out of the 'fmt ' chunk. */                                                            
	if ((nError = mmioAscend(hmmioIn, &ckIn, 0)) != 0)
		{
		goto ERROR_READING_WAVE;

		}
	

	goto TEMPCLEANUP;               

ERROR_READING_WAVE:
	if (*ppwfxInfo != NULL)
		{
		GlobalFree(*ppwfxInfo);
		*ppwfxInfo = NULL;
		}               

	if (hmmioIn != NULL)
	{
	mmioClose(hmmioIn, 0);
		hmmioIn = NULL;
		}
	
TEMPCLEANUP:
	*phmmioIn = hmmioIn;

	return(nError);

}

/*      This routine has to be called before WaveReadFile as it searchs for the chunk to descend into for
	reading, that is, the 'data' chunk.  For simplicity, this used to be in the open routine, but was
	taken out and moved to a separate routine so there was more control on the chunks that are before
	the data chunk, such as 'fact', etc... */

int WaveStartDataRead(
					HMMIO *phmmioIn,
					MMCKINFO *pckIn,
					MMCKINFO *pckInRIFF
					)
{
	int                     nError;

	nError = 0;
	
	// Do a nice little seek...
	if ((nError = mmioSeek(*phmmioIn, pckInRIFF->dwDataOffset + sizeof(FOURCC), SEEK_SET)) == -1)
		{
		Assert(FALSE);
		}

	nError = 0;
	//      Search the input file for for the 'data' chunk.
	pckIn->ckid = mmioFOURCC('d', 'a', 't', 'a');
	if ((nError = mmioDescend(*phmmioIn, pckIn, pckInRIFF, MMIO_FINDCHUNK)) != 0)
		{
		goto ERROR_READING_WAVE;
		}

	goto CLEANUP;

ERROR_READING_WAVE:

CLEANUP:        
	return(nError);
}


/*      This will read wave data from the wave file.  Makre sure we're descended into
	the data chunk, else this will fail bigtime!
	hmmioIn         - Handle to mmio.
	cbRead          - # of bytes to read.   
	pbDest          - Destination buffer to put bytes.
	cbActualRead- # of bytes actually read.

		

*/


int WaveReadFile(
		HMMIO hmmioIn,                          // IN
		UINT cbRead,                            // IN           
		BYTE *pbDest,                           // IN
		MMCKINFO *pckIn,                        // IN.
		UINT *cbActualRead                      // OUT.
		
		)
{

	MMIOINFO    mmioinfoIn;         // current status of <hmmioIn>
	int                     nError;
	UINT                    cT, cbDataIn, uCopyLength;

	nError = 0;

	if (nError = mmioGetInfo(hmmioIn, &mmioinfoIn, 0) != 0)
		{
		goto ERROR_CANNOT_READ;
		}
				
	cbDataIn = cbRead;
	if (cbDataIn > pckIn->cksize) 
		cbDataIn = pckIn->cksize;       

	pckIn->cksize -= cbDataIn;
	
	for (cT = 0; cT < cbDataIn; )
		{
		/* Copy the bytes from the io to the buffer. */
		if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
			{
	    if ((nError = mmioAdvance(hmmioIn, &mmioinfoIn, MMIO_READ)) != 0)
				{
		goto ERROR_CANNOT_READ;
				} 
	    if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
				{
				nError = ER_CORRUPTWAVEFILE;
		goto ERROR_CANNOT_READ;
				}
			}
			
		// Actual copy.
		uCopyLength = (UINT)(mmioinfoIn.pchEndRead - mmioinfoIn.pchNext);
		if((cbDataIn - cT) < uCopyLength )
			uCopyLength = cbDataIn - cT;
		memcpy( (BYTE*)(pbDest+cT), (BYTE*)mmioinfoIn.pchNext, uCopyLength );
		cT += uCopyLength;
		mmioinfoIn.pchNext += uCopyLength;
		}

	if ((nError = mmioSetInfo(hmmioIn, &mmioinfoIn, 0)) != 0)
		{
		goto ERROR_CANNOT_READ;
		}

	*cbActualRead = cbDataIn;
	goto FINISHED_READING;

ERROR_CANNOT_READ:
	*cbActualRead = 0;

FINISHED_READING:
	return(nError);

}

/*      This will close the wave file openned with WaveOpenFile.  
	phmmioIn - Pointer to the handle to input MMIO.
	ppwfxSrc - Pointer to pointer to WaveFormatEx structure.

	Returns 0 if successful, non-zero if there was a warning.

*/
int WaveCloseReadFile(
			HMMIO *phmmio,                                  // IN
			WAVEFORMATEX **ppwfxSrc                 // IN
			)
{

	if (*ppwfxSrc != NULL)
		{
		GlobalFree(*ppwfxSrc);
		*ppwfxSrc = NULL;
		}

	if (*phmmio != NULL)
		{
		mmioClose(*phmmio, 0);
		*phmmio = NULL;
		}

	return(0);

}


};	// end namespace WaveRead
//
//
//

