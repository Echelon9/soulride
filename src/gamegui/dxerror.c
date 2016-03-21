#include "dxerror.h"

/************************************************
 *                                              *
 *  File: dxerror.c                             *
 *  DirectX Error reporting utility             *
 *                                              *
 ************************************************/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
// #include <d3drmwin.h>
#include <d3drm.h>


/*
 *  DXErrorToString
 *  Returns a pointer to a string describing the given DD, D3D or D3DRM error code.
 */

char *DXErrorToString( unsigned long error )
{
    switch( error )
    {
        case DD_OK:
            /* Also includes D3D_OK and D3DRM_OK */
            return "No error.\0";
        case DDERR_ALREADYINITIALIZED:
            return "DDERR_ALREADYINITIALIZED:This object is already initialized.\0";
        case DDERR_BLTFASTCANTCLIP:
            return "DDERR_BLTFASTCANTCLIP:Return if a clipper object is attached to the source surface passed into a BltFast call.\0";
        case DDERR_CANNOTATTACHSURFACE:
            return "DDERR_CANNOTATTACHSURFACE:This surface can not be attached to the requested surface.\0";
        case DDERR_CANNOTDETACHSURFACE:
            return "DDERR_CANNOTDETACHSURFACE:This surface can not be detached from the requested surface.\0";
        case DDERR_CANTCREATEDC:
            return "DDERR_CANTCREATEDC:Windows can not create any more DCs.\0";
        case DDERR_CANTDUPLICATE:
            return "DDERR_CANTDUPLICATE:Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.\0";
        case DDERR_CLIPPERISUSINGHWND:
            return "DDERR_CLIPPERISUSINGHWND:An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.\0";
        case DDERR_COLORKEYNOTSET:
            return "DDERR_COLORKEYNOTSET:No src color key specified for this operation.\0";
        case DDERR_CURRENTLYNOTAVAIL:
            return "DDERR_CURRENTLYNOTAVAIL:Support is currently not available.\0";
        case DDERR_DIRECTDRAWALREADYCREATED:
            return "DDERR_DIRECTDRAWALREADYCREATED:A DirectDraw object representing this driver has already been created for this process.\0";
        case DDERR_EXCEPTION:
            return "DDERR_EXCEPTION:An exception was encountered while performing the requested operation.\0";
        case DDERR_EXCLUSIVEMODEALREADYSET:
            return "DDERR_EXCLUSIVEMODEALREADYSET:An attempt was made to set the cooperative level when it was already set to exclusive.\0";
        case DDERR_GENERIC:
            return "DDERR_GENERIC:Generic failure.\0";
        case DDERR_HEIGHTALIGN:
            return "DDERR_HEIGHTALIGN:Height of rectangle provided is not a multiple of reqd alignment.\0";
        case DDERR_HWNDALREADYSET:
            return "DDERR_HWNDALREADYSET:The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.\0";
        case DDERR_HWNDSUBCLASSED:
            return "DDERR_HWNDSUBCLASSED:HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.\0";
        case DDERR_IMPLICITLYCREATED:
            return "DDERR_IMPLICITLYCREATED:This surface can not be restored because it is an implicitly created surface.\0";
        case DDERR_INCOMPATIBLEPRIMARY:
            return "DDERR_INCOMPATIBLEPRIMARY:Unable to match primary surface creation request with existing primary surface.\0";
        case DDERR_INVALIDCAPS:
            return "DDERR_INVALIDCAPS:One or more of the caps bits passed to the callback are incorrect.\0";
        case DDERR_INVALIDCLIPLIST:
            return "DDERR_INVALIDCLIPLIST:DirectDraw does not support the provided cliplist.\0";
        case DDERR_INVALIDDIRECTDRAWGUID:
            return "DDERR_INVALIDDIRECTDRAWGUID:The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.\0";
        case DDERR_INVALIDMODE:
            return "DDERR_INVALIDMODE:DirectDraw does not support the requested mode.\0";
        case DDERR_INVALIDOBJECT:
            return "DDERR_INVALIDOBJECT:DirectDraw received a pointer that was an invalid DIRECTDRAW object.\0";
        case DDERR_INVALIDPARAMS:
            return "DDERR_INVALIDPARAMS:One or more of the parameters passed to the function are incorrect.\0";
        case DDERR_INVALIDPIXELFORMAT:
            return "DDERR_INVALIDPIXELFORMAT:The pixel format was invalid as specified.\0";
        case DDERR_INVALIDPOSITION:
            return "DDERR_INVALIDPOSITION:Returned when the position of the overlay on the destination is no longer legal for that destination.\0";
        case DDERR_INVALIDRECT:
            return "DDERR_INVALIDRECT:Rectangle provided was invalid.\0";
        case DDERR_LOCKEDSURFACES:
            return "DDERR_LOCKEDSURFACES:Operation could not be carried out because one or more surfaces are locked.\0";
        case DDERR_NO3D:
            return "DDERR_NO3D:There is no 3D present.\0";
        case DDERR_NOALPHAHW:
            return "DDERR_NOALPHAHW:Operation could not be carried out because there is no alpha accleration hardware present or available.\0";
        case DDERR_NOBLTHW:
            return "DDERR_NOBLTHW:No blitter hardware present.\0";
        case DDERR_NOCLIPLIST:
            return "DDERR_NOCLIPLIST:No cliplist available.\0";
        case DDERR_NOCLIPPERATTACHED:
            return "DDERR_NOCLIPPERATTACHED:No clipper object attached to surface object.\0";
        case DDERR_NOCOLORCONVHW:
            return "DDERR_NOCOLORCONVHW:Operation could not be carried out because there is no color conversion hardware present or available.\0";
        case DDERR_NOCOLORKEY:
            return "DDERR_NOCOLORKEY:Surface doesn't currently have a color key\0";
        case DDERR_NOCOLORKEYHW:
            return "DDERR_NOCOLORKEYHW:Operation could not be carried out because there is no hardware support of the destination color key.\0";
        case DDERR_NOCOOPERATIVELEVELSET:
            return "DDERR_NOCOOPERATIVELEVELSET:Create function called without DirectDraw object method SetCooperativeLevel being called.\0";
        case DDERR_NODC:
            return "DDERR_NODC:No DC was ever created for this surface.\0";
        case DDERR_NODDROPSHW:
            return "DDERR_NODDROPSHW:No DirectDraw ROP hardware.\0";
        case DDERR_NODIRECTDRAWHW:
            return "DDERR_NODIRECTDRAWHW:A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.\0";
        case DDERR_NOEMULATION:
            return "DDERR_NOEMULATION:Software emulation not available.\0";
        case DDERR_NOEXCLUSIVEMODE:
            return "DDERR_NOEXCLUSIVEMODE:Operation requires the application to have exclusive mode but the application does not have exclusive mode.\0";
        case DDERR_NOFLIPHW:
            return "DDERR_NOFLIPHW:Flipping visible surfaces is not supported.\0";
        case DDERR_NOGDI:
            return "DDERR_NOGDI:There is no GDI present.\0";
        case DDERR_NOHWND:
            return "DDERR_NOHWND:Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.\0";
        case DDERR_NOMIRRORHW:
            return "DDERR_NOMIRRORHW:Operation could not be carried out because there is no hardware present or available.\0";
        case DDERR_NOOVERLAYDEST:
            return "DDERR_NOOVERLAYDEST:Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.\0";
        case DDERR_NOOVERLAYHW:
            return "DDERR_NOOVERLAYHW:Operation could not be carried out because there is no overlay hardware present or available.\0";
        case DDERR_NOPALETTEATTACHED:
            return "DDERR_NOPALETTEATTACHED:No palette object attached to this surface.\0";
        case DDERR_NOPALETTEHW:
            return "DDERR_NOPALETTEHW:No hardware support for 16 or 256 color palettes.\0";
        case DDERR_NORASTEROPHW:
            return "DDERR_NORASTEROPHW:Operation could not be carried out because there is no appropriate raster op hardware present or available.\0";
        case DDERR_NOROTATIONHW:
            return "DDERR_NOROTATIONHW:Operation could not be carried out because there is no rotation hardware present or available.\0";
        case DDERR_NOSTRETCHHW:
            return "DDERR_NOSTRETCHHW:Operation could not be carried out because there is no hardware support for stretching.\0";
        case DDERR_NOT4BITCOLOR:
            return "DDERR_NOT4BITCOLOR:DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.\0";
        case DDERR_NOT4BITCOLORINDEX:
            return "DDERR_NOT4BITCOLORINDEX:DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.\0";
        case DDERR_NOT8BITCOLOR:
            return "DDERR_NOT8BITCOLOR:DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.\0";
        case DDERR_NOTAOVERLAYSURFACE:
            return "DDERR_NOTAOVERLAYSURFACE:Returned when an overlay member is called for a non-overlay surface.\0";
        case DDERR_NOTEXTUREHW:
            return "DDERR_NOTEXTUREHW:Operation could not be carried out because there is no texture mapping hardware present or available.\0";
        case DDERR_NOTFLIPPABLE:
            return "DDERR_NOTFLIPPABLE:An attempt has been made to flip a surface that is not flippable.\0";
        case DDERR_NOTFOUND:
            return "DDERR_NOTFOUND:Requested item was not found.\0";
        case DDERR_NOTLOCKED:
            return "DDERR_NOTLOCKED:Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.\0";
        case DDERR_NOTPALETTIZED:
            return "DDERR_NOTPALETTIZED:The surface being used is not a palette-based surface.\0";
        case DDERR_NOVSYNCHW:
            return "DDERR_NOVSYNCHW:Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.\0";
        case DDERR_NOZBUFFERHW:
            return "DDERR_NOZBUFFERHW:Operation could not be carried out because there is no hardware support for zbuffer blitting.\0";
        case DDERR_NOZOVERLAYHW:
            return "DDERR_NOZOVERLAYHW:Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.\0";
        case DDERR_OUTOFCAPS:
            return "DDERR_OUTOFCAPS:The hardware needed for the requested operation has already been allocated.\0";
        case DDERR_OUTOFMEMORY:
            return "DDERR_OUTOFMEMORY:DirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OUTOFVIDEOMEMORY:
            return "DDERR_OUTOFVIDEOMEMORY:DirectDraw does not have enough memory to perform the operation.\0";
        case DDERR_OVERLAYCANTCLIP:
            return "DDERR_OVERLAYCANTCLIP:The hardware does not support clipped overlays.\0";
        case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
            return "DDERR_OVERLAYCOLORKEYONLYONEACTIVE:Can only have ony color key active at one time for overlays.\0";
        case DDERR_OVERLAYNOTVISIBLE:
            return "DDERR_OVERLAYNOTVISIBLE:Returned when GetOverlayPosition is called on a hidden overlay.\0";
        case DDERR_PALETTEBUSY:
            return "DDERR_PALETTEBUSY:Access to this palette is being refused because the palette is already locked by another thread.\0";
        case DDERR_PRIMARYSURFACEALREADYEXISTS:
            return "DDERR_PRIMARYSURFACEALREADYEXISTS:This process already has created a primary surface.\0";
        case DDERR_REGIONTOOSMALL:
            return "DDERR_REGIONTOOSMALL:Region passed to Clipper::GetClipList is too small.\0";
        case DDERR_SURFACEALREADYATTACHED:
            return "DDERR_SURFACEALREADYATTACHED:This surface is already attached to the surface it is being attached to.\0";
        case DDERR_SURFACEALREADYDEPENDENT:
            return "DDERR_SURFACEALREADYDEPENDENT:This surface is already a dependency of the surface it is being made a dependency of.\0";
        case DDERR_SURFACEBUSY:
            return "DDERR_SURFACEBUSY:Access to this surface is being refused because the surface is already locked by another thread.\0";
        case DDERR_SURFACEISOBSCURED:
            return "DDERR_SURFACEISOBSCURED:Access to surface refused because the surface is obscured.\0";
        case DDERR_SURFACELOST:
            return "DDERR_SURFACELOST:Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.\0";
        case DDERR_SURFACENOTATTACHED:
            return "DDERR_SURFACENOTATTACHED:The requested surface is not attached.\0";
        case DDERR_TOOBIGHEIGHT:
            return "DDERR_TOOBIGHEIGHT:Height requested by DirectDraw is too large.\0";
        case DDERR_TOOBIGSIZE:
            return "DDERR_TOOBIGSIZE:Size requested by DirectDraw is too large, but the individual height and width are OK.\0";
        case DDERR_TOOBIGWIDTH:
            return "DDERR_TOOBIGWIDTH:Width requested by DirectDraw is too large.\0";
        case DDERR_UNSUPPORTED:
            return "DDERR_UNSUPPORTED:Action not supported.\0";
        case DDERR_UNSUPPORTEDFORMAT:
            return "DDERR_UNSUPPORTEDFORMAT:FOURCC format requested is unsupported by DirectDraw.\0";
        case DDERR_UNSUPPORTEDMASK:
            return "DDERR_UNSUPPORTEDMASK:Bitmask in the pixel format requested is unsupported by DirectDraw.\0";
        case DDERR_VERTICALBLANKINPROGRESS:
            return "DDERR_VERTICALBLANKINPROGRESS:Vertical blank is in progress.\0";
        case DDERR_WASSTILLDRAWING:
            return "DDERR_WASSTILLDRAWING:The previous Blt which is transfering information to or from this Surface is incomplete.\0";
        case DDERR_WRONGMODE:
            return "DDERR_WRONGMODE:This surface can not be restored because it was created in a different mode.\0";
        case DDERR_XALIGN:
            return "DDERR_XALIGN:Rectangle provided was not horizontally aligned on required boundary.\0";
        case D3DERR_BADMAJORVERSION:
            return "D3DERR_BADMAJORVERSION\0";
        case D3DERR_BADMINORVERSION:
            return "D3DERR_BADMINORVERSION\0";
        case D3DERR_EXECUTE_LOCKED:
            return "D3DERR_EXECUTE_LOCKED\0";
        case D3DERR_EXECUTE_NOT_LOCKED:
            return "D3DERR_EXECUTE_NOT_LOCKED\0";
        case D3DERR_EXECUTE_CREATE_FAILED:
            return "D3DERR_EXECUTE_CREATE_FAILED\0";
        case D3DERR_EXECUTE_DESTROY_FAILED:
            return "D3DERR_EXECUTE_DESTROY_FAILED\0";
        case D3DERR_EXECUTE_LOCK_FAILED:
            return "D3DERR_EXECUTE_LOCK_FAILED\0";
        case D3DERR_EXECUTE_UNLOCK_FAILED:
            return "D3DERR_EXECUTE_UNLOCK_FAILED\0";
        case D3DERR_EXECUTE_FAILED:
            return "D3DERR_EXECUTE_FAILED\0";
        case D3DERR_EXECUTE_CLIPPED_FAILED:
            return "D3DERR_EXECUTE_CLIPPED_FAILED\0";
        case D3DERR_TEXTURE_NO_SUPPORT:
            return "D3DERR_TEXTURE_NO_SUPPORT\0";
        case D3DERR_TEXTURE_NOT_LOCKED:
            return "D3DERR_TEXTURE_NOT_LOCKED\0";
        case D3DERR_TEXTURE_LOCKED:
            return "D3DERR_TEXTURELOCKED\0";
        case D3DERR_TEXTURE_CREATE_FAILED:
            return "D3DERR_TEXTURE_CREATE_FAILED\0";
        case D3DERR_TEXTURE_DESTROY_FAILED:
            return "D3DERR_TEXTURE_DESTROY_FAILED\0";
        case D3DERR_TEXTURE_LOCK_FAILED:
            return "D3DERR_TEXTURE_LOCK_FAILED\0";
        case D3DERR_TEXTURE_UNLOCK_FAILED:
            return "D3DERR_TEXTURE_UNLOCK_FAILED\0";
        case D3DERR_TEXTURE_LOAD_FAILED:
            return "D3DERR_TEXTURE_LOAD_FAILED\0";
        case D3DERR_MATRIX_CREATE_FAILED:
            return "D3DERR_MATRIX_CREATE_FAILED\0";
        case D3DERR_MATRIX_DESTROY_FAILED:
            return "D3DERR_MATRIX_DESTROY_FAILED\0";
        case D3DERR_MATRIX_SETDATA_FAILED:
            return "D3DERR_MATRIX_SETDATA_FAILED\0";
        case D3DERR_SETVIEWPORTDATA_FAILED:
            return "D3DERR_SETVIEWPORTDATA_FAILED\0";
        case D3DERR_MATERIAL_CREATE_FAILED:
            return "D3DERR_MATERIAL_CREATE_FAILED\0";
        case D3DERR_MATERIAL_DESTROY_FAILED:
            return "D3DERR_MATERIAL_DESTROY_FAILED\0";
        case D3DERR_MATERIAL_SETDATA_FAILED:
            return "D3DERR_MATERIAL_SETDATA_FAILED\0";
        case D3DERR_LIGHT_SET_FAILED:
            return "D3DERR_LIGHT_SET_FAILED\0";
        case D3DERR_INVALIDRAMPTEXTURE:
            return "D3DERR_INVALIDRAMPTEXTURE\0";
        case D3DRMERR_BADOBJECT:
            return "D3DRMERR_BADOBJECT\0";
        case D3DRMERR_BADTYPE:
            return "D3DRMERR_BADTYPE\0";
        case D3DRMERR_BADALLOC:
            return "D3DRMERR_BADALLOC\0";
        case D3DRMERR_FACEUSED:
            return "D3DRMERR_FACEUSED\0";
        case D3DRMERR_NOTFOUND:
            return "D3DRMERR_NOTFOUND\0";
        case D3DRMERR_NOTDONEYET:
            return "D3DRMERR_NOTDONEYET\0";
        case D3DRMERR_FILENOTFOUND:
            return "The file was not found.\0";
        case D3DRMERR_BADFILE:
            return "D3DRMERR_BADFILE\0";
        case D3DRMERR_BADDEVICE:
            return "D3DRMERR_BADDEVICE\0";
        case D3DRMERR_BADVALUE:
            return "D3DRMERR_BADVALUE\0";
        case D3DRMERR_BADMAJORVERSION:
            return "D3DRMERR_BADMAJORVERSION\0";
        case D3DRMERR_BADMINORVERSION:
            return "D3DRMERR_BADMINORVERSION\0";
        case D3DRMERR_UNABLETOEXECUTE:
            return "D3DRMERR_UNABLETOEXECUTE\0";
        default:
            return "Unrecognized DirectX error value.\0";
    }
}
