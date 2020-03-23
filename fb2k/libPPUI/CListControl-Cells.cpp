#include "stdafx.h"
#include "CListControl.h"
#include "CListControlHeaderImpl.h"
#include "CListControl-Cells.h"
#include "PaintUtils.h"
#include "GDIUtils.h"
#include <vsstyle.h>

/* FIX ME LIST 
Cell disabled state (from disabled window), drop window arg?
Fewer stupid methods ?
*/

LONG CListCell::AccRole() {
	return ROLE_SYSTEM_LISTITEM;
}

void RenderCheckbox( HTHEME theme, CWindow wnd, CDCHandle dc, CRect rcCheckBox, unsigned stateFlags, bool bRadio ) {

	const int part = bRadio ? BP_RADIOBUTTON : BP_CHECKBOX;

	const bool bDisabled = ! wnd.IsWindowEnabled();
	const bool bPressed = (stateFlags & CListCell::cellState_pressed ) != 0;
	const bool bHot = ( stateFlags & CListCell::cellState_hot ) != 0;

	if (theme != NULL && IsThemePartDefined(theme, part, 0)) {
		int state = 0;
		if (bDisabled) {
			state = bPressed ? CBS_CHECKEDDISABLED : CBS_DISABLED;
		} else if ( bHot ) {
			state = bPressed ? CBS_CHECKEDHOT : CBS_HOT;
		} else {
			state = bPressed ? CBS_CHECKEDNORMAL : CBS_NORMAL;
		}

		CSize size;
		if (SUCCEEDED(GetThemePartSize(theme, dc, part, state, rcCheckBox, TS_TRUE, &size))) {
			if (size.cx <= rcCheckBox.Width() && size.cy <= rcCheckBox.Height()) {
				CRect rc = rcCheckBox;
				rc.left += ( rc.Width() - size.cx ) / 2;
				rc.top += ( rc.Height() - size.cy ) / 2;
				rc.right = rc.left + size.cx;
				rc.bottom = rc.top + size.cy;
				DrawThemeBackground(theme, dc, part, state, rc, &rc);
				return;
			}
		}
	}
	int stateEx = bRadio ? DFCS_BUTTONRADIO : DFCS_BUTTONCHECK;
	if ( bPressed ) stateEx |= DFCS_CHECKED;
	if ( bDisabled ) stateEx |= DFCS_INACTIVE;
	else if ( bHot ) stateEx |= DFCS_HOT;
	DrawFrameControl(dc, rcCheckBox, DFC_BUTTON, stateEx);
}

void RenderButton( HTHEME theme, CWindow wnd, CDCHandle dc, CRect rcButton, CRect rcUpdate, uint32_t cellState ) {

	const int part = BP_PUSHBUTTON;

	enum {
		stNormal = PBS_NORMAL,
		stHot = PBS_HOT,
		stDisabled = PBS_DISABLED,
		stPressed = PBS_PRESSED,
	};

	int state = 0;
	if (!wnd.IsWindowEnabled()) state = stDisabled;
	if ( cellState & CListCell::cellState_pressed ) state = stPressed;
	else if ( cellState & CListCell::cellState_hot ) state = stHot;
	else state = stNormal;

	CRect rcClient  = rcButton;

	if (theme != NULL && IsThemePartDefined(theme, part, 0)) {
		DrawThemeBackground(theme, dc, part, state, rcClient, &rcUpdate);
	} else {
		int stateEx = DFCS_BUTTONPUSH;
		switch (state) {
		case stPressed: stateEx |= DFCS_PUSHED; break;
		case stDisabled: stateEx |= DFCS_INACTIVE; break;
		}
		DrawFrameControl(dc, rcClient, DFC_BUTTON, stateEx);
	}
}

bool CListCell::ApplyTextStyle( LOGFONT & font, double scale, uint32_t ) { 
	if ( scale != 1.0 ) {
		font.lfHeight = pfc::rint32( font.lfHeight * scale );
		return true;
	} else {
		return false;
	}
}

void CListCell_Text::DrawContent( DrawContentArg_t const & arg ) {
	CDCHandle dc = arg.dc;
	
	CRect clip = arg.rcText;

	const t_uint32 format = PaintUtils::DrawText_TranslateHeaderAlignment(arg.hdrFormat);
	dc.DrawText( arg.text, (int)wcslen(arg.text), clip, format | DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER  );
}

void CListCell_TextColors::DrawContent( DrawContentArg_t const & arg ) {
	CDCHandle dc = arg.dc;
	
	CRect clip = arg.rcText;

	const uint32_t fgWas = dc.GetTextColor();

	const t_uint32 format = PaintUtils::DrawText_TranslateHeaderAlignment(arg.hdrFormat);
	const t_uint32 bk = dc.GetBkColor();
	const t_uint32 fg = fgWas;
	const t_uint32 hl = (arg.allowColors ? arg.colorHighlight : fg);
	const t_uint32 colors[3] = { PaintUtils::BlendColor(bk, fg, 33), fg, hl };

	PaintUtils::TextOutColorsEx(dc, arg.text, clip, format, colors);

	dc.SetTextColor(fgWas);
}

void CListCell_MultiText::DrawContent( DrawContentArg_t const & arg ) {
	CDCHandle dc = arg.dc;

	const int textLen = (int) wcslen( arg.text );

	CRect clip = arg.rcText;

	const t_uint32 format = PaintUtils::DrawText_TranslateHeaderAlignment(arg.hdrFormat) | DT_NOPREFIX | DT_VCENTER ;

	CRect rcDraw = clip;
	dc.DrawText(arg.text, textLen, rcDraw, format | DT_CALCRECT);
	auto txSize = rcDraw.Size();
	rcDraw = clip;
	if ( txSize.cy < rcDraw.Height() ) {
		int sub = rcDraw.Height() - txSize.cy;
		rcDraw.top += sub/2;
		rcDraw.bottom = rcDraw.top + txSize.cy;
	}
	dc.DrawText(arg.text, textLen, rcDraw, format);
}

bool CListCell_Hyperlink::ApplyTextStyle( LOGFONT & font, double scale, uint32_t state ) {
	bool rv = __super::ApplyTextStyle(font, scale, state);

	if ( state & cellState_hot ) {
		font.lfUnderline = TRUE;
		rv = true;
	}

	return rv;
}

HCURSOR CListCell_Hyperlink::HotCursor() {
	return LoadCursor(NULL, IDC_HAND);
}

LONG CListCell_Hyperlink::AccRole() {
	return ROLE_SYSTEM_LINK;
}

void CListCell_Hyperlink::DrawContent( DrawContentArg_t const & arg ) {

	CDCHandle dc = arg.dc;

	const uint32_t fgWas = dc.GetTextColor();

	const t_uint32 format = PaintUtils::DrawText_TranslateHeaderAlignment(arg.hdrFormat);
	if (arg.allowColors) dc.SetTextColor( arg.colorHighlight );
	const t_uint32 bk = dc.GetBkColor();

	CRect rc = arg.rcText;
	dc.DrawText(arg.text, (int) wcslen(arg.text), rc, format | DT_NOPREFIX | DT_END_ELLIPSIS | DT_SINGLELINE | DT_VCENTER );

	dc.SetTextColor(fgWas);
}

LONG CListCell_Button::AccRole() {
	return ROLE_SYSTEM_PUSHBUTTON;
}

void CListCell_Button::DrawContent( DrawContentArg_t const & arg ) {

	CDCHandle dc = arg.dc;

	const bool bPressed = (arg.cellState & cellState_pressed) != 0;
	const bool bHot = (arg.cellState & cellState_hot) != 0;

	
	if ( !m_lite || bHot || bPressed ) {
		RenderButton( arg.theme, arg.thisWnd, dc, arg.rcHot, arg.rcHot, arg.cellState );
	}

	CRect clip = arg.rcText;

	dc.DrawText(arg.text, (int) wcslen(arg.text), clip, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_CENTER);
}

bool CListCell_ButtonGlyph::ApplyTextStyle( LOGFONT & font, double scale, uint32_t state ) {
	return __super::ApplyTextStyle(font, scale * 1.3, state);
}

static CRect CheckBoxRect(CRect rc) {
	if (rc.Width() > rc.Height()) {
		rc.right = rc.left + rc.Height();
	}
	return rc;
}

LONG CListCell_Checkbox::AccRole()  {
	return m_radio ? ROLE_SYSTEM_RADIOBUTTON : ROLE_SYSTEM_CHECKBUTTON;
}

CRect CListCell_Checkbox::HotRect( CRect rc ) {
	return CheckBoxRect( rc );
}

void CListCell_Checkbox::DrawContent( DrawContentArg_t const & arg ) {
	
	CDCHandle dc = arg.dc;
	
	const bool bPressed = (arg.cellState & cellState_pressed) != 0;
	const bool bHot = (arg.cellState & cellState_hot) != 0;
	

	CRect clip = arg.rcText;

	const uint32_t fgWas = dc.GetTextColor();

	if (arg.subItemRect.Width() > arg.subItemRect.Height() ) {
		CRect rcCheckbox = arg.subItemRect;
		rcCheckbox.right = rcCheckbox.left + rcCheckbox.Height();
		RenderCheckbox(arg.theme, arg.thisWnd, dc, rcCheckbox, arg.cellState, m_radio );
		CRect rcText = arg.subItemRect;
		rcText.left = rcCheckbox.right;
		dc.DrawText(arg.text, (int) wcslen(arg.text), rcText, DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER | DT_LEFT);
	} else {
		RenderCheckbox(arg.theme, arg.thisWnd, dc, arg.subItemRect, arg.cellState, m_radio );
	}

}
