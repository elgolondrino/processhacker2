/*
 * Process Hacker ToolStatus -
 *   Toolbar Customize Dialog
 *
 * Copyright (C) 2015-2016 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "toolstatus.h"
#include "commonutil.h"

static PWSTR CustomizeTextOptionsStrings[] =
{
    L"No text labels",
    L"Selective text",
    L"Show text labels"
};

static PWSTR CustomizeSearchDisplayStrings[] =
{
    L"Always show",
    L"Hide when inactive (Ctrl+K)",
    // L"Auto-hide"
};

static PWSTR CustomizeThemeOptionsStrings[] =
{
    L"None",
    L"Black",
    L"Blue"
};

BOOLEAN CustomizeToolbarItemExists(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IdCommand
    )
{
    INT buttonIndex = 0;
    INT buttonCount = 0;

    buttonCount = ListBox_GetCount(Context->CurrentListHandle);

    if (buttonCount == LB_ERR)
        return FALSE;

    for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
    {
        PBUTTON_CONTEXT itemContext;

        itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, buttonIndex);

        if (itemContext == NULL)
            continue;

        if (itemContext->IdCommand == IdCommand)
            return TRUE;
    }

    return FALSE;
}

VOID CustomizeInsertToolbarButton(
    _In_ INT Index,
    _In_ PBUTTON_CONTEXT ItemContext
    )
{
    TBBUTTON button;

    memset(&button, 0, sizeof(TBBUTTON));

    button.idCommand = ItemContext->IdCommand;
    button.iBitmap = I_IMAGECALLBACK;
    button.fsState = TBSTATE_ENABLED;
    button.fsStyle = ItemContext->IsSeparator ? BTNS_SEP : BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT;
    button.iString = (INT_PTR)ToolbarGetText(ItemContext->IdCommand);

    SendMessage(ToolBarHandle, TB_INSERTBUTTON, Index, (LPARAM)&button);
}

VOID CustomizeAddToolbarItem(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IndexAvail,
    _In_ INT IndexTo
    )
{
    INT count;
    PBUTTON_CONTEXT itemContext;

    count = ListBox_GetCount(Context->AvailableListHandle);
    itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, IndexAvail);

    if (count == LB_ERR)
        return;

    if (itemContext == NULL)
        return;

    if (IndexAvail != 0) // index 0 is separator
    {
        // remove from 'available' list
        ListBox_DeleteString(Context->AvailableListHandle, IndexAvail);

        if (IndexAvail == count - 1)
        {
            ListBox_SetCurSel(Context->AvailableListHandle, IndexAvail - 1);
        }
        else
        {
            ListBox_SetCurSel(Context->AvailableListHandle, IndexAvail);
        }
    }
    else
    {
        itemContext = PhAllocate(sizeof(BUTTON_CONTEXT));
        memset(itemContext, 0, sizeof(BUTTON_CONTEXT));

        itemContext->IsSeparator = TRUE;
        itemContext->IsRemovable = TRUE;
    }

    // insert into 'current' list
    ListBox_InsertItemData(Context->CurrentListHandle, IndexTo, itemContext);

    CustomizeInsertToolbarButton(IndexTo, itemContext);
}

VOID CustomizeRemoveToolbarItem(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IndexFrom
    )
{
    PBUTTON_CONTEXT itemContext;

    itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, IndexFrom);

    if (itemContext == NULL)
        return;

    ListBox_DeleteString(Context->CurrentListHandle, IndexFrom);
    ListBox_SetCurSel(Context->CurrentListHandle, IndexFrom);

    SendMessage(ToolBarHandle, TB_DELETEBUTTON, IndexFrom, 0);

    if (itemContext->IsSeparator)
    {
        PhFree(itemContext);
    }
    else
    {
        // insert into 'available' list
        ListBox_AddItemData(Context->AvailableListHandle, itemContext);
    }

    SendMessage(Context->DialogHandle, WM_COMMAND, MAKEWPARAM(IDC_CURRENT, LBN_SELCHANGE), 0);
}

VOID CustomizeMoveToolbarItem(
    _In_ PCUSTOMIZE_CONTEXT Context,
    _In_ INT IndexFrom,
    _In_ INT IndexTo
    )
{
    INT count;
    PBUTTON_CONTEXT itemContext;

    if (IndexFrom == IndexTo)
        return;

    count = ListBox_GetCount(Context->CurrentListHandle);
    itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, IndexFrom);

    if (count == LB_ERR)
        return;

    if (itemContext == NULL)
        return;

    ListBox_DeleteString(Context->CurrentListHandle, IndexFrom);
    ListBox_InsertItemData(Context->CurrentListHandle, IndexTo, itemContext);
    ListBox_SetCurSel(Context->CurrentListHandle, IndexTo);

    if (IndexTo <= 0)
    {
        Button_Enable(Context->MoveUpButtonHandle, FALSE);
    }
    else
    {
        Button_Enable(Context->MoveUpButtonHandle, TRUE);
    }

    // last item is always separator
    if (IndexTo >= (count - 2))
    {
        Button_Enable(Context->MoveDownButtonHandle, FALSE);
    }
    else
    {
        Button_Enable(Context->MoveDownButtonHandle, TRUE);
    }

    SendMessage(ToolBarHandle, TB_DELETEBUTTON, IndexFrom, 0);

    CustomizeInsertToolbarButton(IndexTo, itemContext);
}

VOID CustomizeFreeToolbarItems(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    INT buttonIndex = 0;
    INT buttonCount = 0;

    buttonCount = ListBox_GetCount(Context->CurrentListHandle);

    if (buttonCount != LB_ERR)
    {
        for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
        {
            PBUTTON_CONTEXT itemContext;

            if (itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, buttonIndex))
            {
                PhFree(itemContext);
            }
        }
    }

    buttonCount = ListBox_GetCount(Context->AvailableListHandle);

    if (buttonCount != LB_ERR)
    {
        for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
        {
            PBUTTON_CONTEXT itemContext;

            if (itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, buttonIndex))
            {
                PhFree(itemContext);
            }
        }
    }
}

VOID CustomizeLoadToolbarItems(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    INT buttonIndex = 0;
    INT buttonCount = 0;
    PBUTTON_CONTEXT itemContext;

    CustomizeFreeToolbarItems(Context);

    ListBox_ResetContent(Context->AvailableListHandle);
    ListBox_ResetContent(Context->CurrentListHandle);

    buttonCount = (INT)SendMessage(ToolBarHandle, TB_BUTTONCOUNT, 0, 0);

    for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
    {
        TBBUTTON button;

        memset(&button, 0, sizeof(TBBUTTON));

        if (SendMessage(ToolBarHandle, TB_GETBUTTON, buttonIndex, (LPARAM)&button))
        {
            itemContext = PhAllocate(sizeof(BUTTON_CONTEXT));
            memset(itemContext, 0, sizeof(BUTTON_CONTEXT));

            itemContext->IsVirtual = FALSE;
            itemContext->IsRemovable = TRUE;
            itemContext->IdCommand = button.idCommand;

            if (button.fsStyle & BTNS_SEP)
            {
                itemContext->IsSeparator = TRUE;
            }
            else
            {
                HBITMAP buttonImage;

                if (buttonImage = ToolbarGetImage(button.idCommand))
                {
                    itemContext->IdBitmap = ImageList_Add(
                        Context->ImageListHandle,
                        buttonImage,
                        NULL
                        );

                    DeleteObject(buttonImage);
                }
            }

            ListBox_AddItemData(Context->CurrentListHandle, itemContext);
        }
    }

    for (buttonIndex = 0; buttonIndex < MAX_TOOLBAR_ITEMS; buttonIndex++)
    {
        HBITMAP buttonImage;
        TBBUTTON button = ToolbarButtons[buttonIndex];

        if (button.idCommand == 0)
            continue;

        if (CustomizeToolbarItemExists(Context, button.idCommand))
            continue;

        // HACK and violation of abstraction.
        // Don't show the 'Show Details for All Processes' button on XP.
        if (!WINDOWS_HAS_UAC && button.idCommand == PHAPP_ID_HACKER_SHOWDETAILSFORALLPROCESSES)
        {
            continue;
        }

        itemContext = PhAllocate(sizeof(BUTTON_CONTEXT));
        memset(itemContext, 0, sizeof(BUTTON_CONTEXT));

        itemContext->IsRemovable = TRUE;
        itemContext->IdCommand = button.idCommand;

        if (buttonImage = ToolbarGetImage(button.idCommand))
        {
            itemContext->IdBitmap = ImageList_Add(
                Context->ImageListHandle,
                buttonImage,
                NULL
                );
            DeleteObject(buttonImage);
        }

        ListBox_AddItemData(Context->AvailableListHandle, itemContext);
    }

    // Append separator to the last 'current list'  position
    itemContext = PhAllocate(sizeof(BUTTON_CONTEXT));
    memset(itemContext, 0, sizeof(BUTTON_CONTEXT));
    itemContext->IsSeparator = TRUE;
    itemContext->IsVirtual = TRUE;
    itemContext->IsRemovable = FALSE;

    buttonIndex = ListBox_AddItemData(Context->CurrentListHandle, itemContext);
    ListBox_SetCurSel(Context->CurrentListHandle, buttonIndex);
    ListBox_SetTopIndex(Context->CurrentListHandle, buttonIndex);

    // Insert separator into first 'available list' position
    itemContext = PhAllocate(sizeof(BUTTON_CONTEXT));
    memset(itemContext, 0, sizeof(BUTTON_CONTEXT));
    itemContext->IsSeparator = TRUE;
    itemContext->IsVirtual = FALSE;
    itemContext->IsRemovable = FALSE;

    buttonIndex = ListBox_InsertItemData(Context->AvailableListHandle, 0, itemContext);
    ListBox_SetCurSel(Context->AvailableListHandle, buttonIndex);
    ListBox_SetTopIndex(Context->AvailableListHandle, buttonIndex);

    // Disable buttons
    Button_Enable(Context->MoveUpButtonHandle, FALSE);
    Button_Enable(Context->MoveDownButtonHandle, FALSE);
    Button_Enable(Context->RemoveButtonHandle, FALSE);
}

VOID CustomizeLoadToolbarSettings(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    HWND toolbarCombo = GetDlgItem(Context->DialogHandle, IDC_TEXTOPTIONS);
    HWND searchboxCombo = GetDlgItem(Context->DialogHandle, IDC_SEARCHOPTIONS);
    HWND themeCombo = GetDlgItem(Context->DialogHandle, IDC_THEMEOPTIONS);

    PhAddComboBoxStrings(
        toolbarCombo,
        CustomizeTextOptionsStrings,
        ARRAYSIZE(CustomizeTextOptionsStrings)
        );
    PhAddComboBoxStrings(
        searchboxCombo,
        CustomizeSearchDisplayStrings,
        ARRAYSIZE(CustomizeSearchDisplayStrings)
        );
    PhAddComboBoxStrings(
        themeCombo,
        CustomizeThemeOptionsStrings,
        ARRAYSIZE(CustomizeThemeOptionsStrings)
        );
    ComboBox_SetCurSel(toolbarCombo, PhGetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE));
    ComboBox_SetCurSel(searchboxCombo, PhGetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE));
    ComboBox_SetCurSel(themeCombo, PhGetIntegerSetting(SETTING_NAME_TOOLBAR_THEME));

    Button_SetCheck(GetDlgItem(Context->DialogHandle, IDC_ENABLE_MODERN),
        ToolStatusConfig.ModernIcons ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(GetDlgItem(Context->DialogHandle, IDC_ENABLE_AUTOHIDE_MENU),
        ToolStatusConfig.AutoHideMenu ? BST_CHECKED : BST_UNCHECKED);

    if (!ToolStatusConfig.SearchBoxEnabled)
    {
        ComboBox_Enable(searchboxCombo, FALSE);
    }

    ComboBox_Enable(themeCombo, FALSE);
}

VOID CustomizeResetImages(
    _In_ PCUSTOMIZE_CONTEXT Context
    )
{
    INT buttonIndex = 0;
    INT buttonCount = 0;

    buttonCount = ListBox_GetCount(Context->CurrentListHandle);

    if (buttonCount != LB_ERR)
    {
        for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
        {
            PBUTTON_CONTEXT itemContext;
            HBITMAP buttonImage;

            if (itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->CurrentListHandle, buttonIndex))
            {
                if (buttonImage = ToolbarGetImage(itemContext->IdCommand))
                {
                    ImageList_Replace(
                        Context->ImageListHandle,
                        itemContext->IdBitmap,
                        buttonImage,
                        NULL
                        );
                    DeleteObject(buttonImage);
                }
            }
        }
    }

    buttonCount = ListBox_GetCount(Context->AvailableListHandle);

    if (buttonCount != LB_ERR)
    {
        for (buttonIndex = 0; buttonIndex < buttonCount; buttonIndex++)
        {
            PBUTTON_CONTEXT itemContext;
            HBITMAP buttonImage;

            if (itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(Context->AvailableListHandle, buttonIndex))
            {
                if (buttonImage = ToolbarGetImage(itemContext->IdCommand))
                {
                    ImageList_Replace(
                        Context->ImageListHandle,
                        itemContext->IdBitmap,
                        buttonImage,
                        NULL
                        );
                    DeleteObject(buttonImage);
                }
            }
        }
    }

    InvalidateRect(Context->AvailableListHandle, NULL, TRUE);
    InvalidateRect(Context->CurrentListHandle, NULL, TRUE);
}

VOID CustomizeResetToolbarImages(
    VOID
    )
{
    // Reset the image cache with the new icons.
    // TODO: Move function to Toolbar.c
    for (INT i = 0; i < ARRAYSIZE(ToolbarButtons); i++)
    {
        if (ToolbarButtons[i].iBitmap != I_IMAGECALLBACK)
        {
            HBITMAP buttonImage;

            if (buttonImage = ToolbarGetImage(ToolbarButtons[i].idCommand))
            {
                ImageList_Replace(
                    ToolBarImageList,
                    ToolbarButtons[i].iBitmap,
                    buttonImage,
                    NULL
                    );

                DeleteObject(buttonImage);
            }
        }
    }
}

INT_PTR CALLBACK CustomizeToolbarDialogProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    PCUSTOMIZE_CONTEXT context = NULL;

    if (uMsg == WM_INITDIALOG)
    {
        context = (PCUSTOMIZE_CONTEXT)PhAllocate(sizeof(CUSTOMIZE_CONTEXT));
        memset(context, 0, sizeof(CUSTOMIZE_CONTEXT));

        SetProp(hwndDlg, L"Context", (HANDLE)context);
    }
    else
    {
        context = (PCUSTOMIZE_CONTEXT)GetProp(hwndDlg, L"Context");

        if (uMsg == WM_NCDESTROY)
        {
            RemoveProp(hwndDlg, L"Context");
            PhFree(context);
        }
    }

    if (context == NULL)
        return FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            PhCenterWindow(hwndDlg, PhMainWndHandle);

            context->DialogHandle = hwndDlg;
            context->AvailableListHandle = GetDlgItem(hwndDlg, IDC_AVAILABLE);
            context->CurrentListHandle = GetDlgItem(hwndDlg, IDC_CURRENT);
            context->MoveUpButtonHandle = GetDlgItem(hwndDlg, IDC_MOVEUP);
            context->MoveDownButtonHandle = GetDlgItem(hwndDlg, IDC_MOVEDOWN);
            context->AddButtonHandle = GetDlgItem(hwndDlg, IDC_ADD);
            context->RemoveButtonHandle = GetDlgItem(hwndDlg, IDC_REMOVE);
            context->BitmapWidth = GetSystemMetrics(SM_CYSMICON) + 4;
            context->FontHandle = CommonDuplicateFont((HFONT)SendMessage(ToolBarHandle, WM_GETFONT, 0, 0));
            context->ImageListHandle = ImageList_Create(
                GetSystemMetrics(SM_CXSMICON),
                GetSystemMetrics(SM_CYSMICON),
                ILC_COLOR32 | ILC_MASK,
                0,
                0
                );

            ListBox_SetItemHeight(context->AvailableListHandle, 0, context->BitmapWidth); // BitmapHeight
            ListBox_SetItemHeight(context->CurrentListHandle, 0, context->BitmapWidth); // BitmapHeight

            CustomizeLoadToolbarItems(context);
            CustomizeLoadToolbarSettings(context);

            SendMessage(context->DialogHandle, WM_NEXTDLGCTL, (WPARAM)context->CurrentListHandle, TRUE);
        }
        return TRUE;
    case WM_DESTROY:
        {
            ToolbarSaveButtonSettings();
            ToolbarLoadSettings();

            CustomizeFreeToolbarItems(context);

            if (context->ImageListHandle)
            {
                ImageList_Destroy(context->ImageListHandle);
            }

            if (context->FontHandle)
            {
                DeleteObject(context->FontHandle);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_AVAILABLE:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case LBN_DBLCLK:
                        {
                            INT index;
                            INT indexto;

                            index = ListBox_GetCurSel(context->AvailableListHandle);
                            indexto = ListBox_GetCurSel(context->CurrentListHandle);

                            if (index == LB_ERR)
                                break;

                            if (indexto == LB_ERR)
                                break;

                            CustomizeAddToolbarItem(context, index, indexto);
                        }
                        break;
                    }
                }
                break;
            case IDC_CURRENT:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                    case LBN_SELCHANGE:
                        {
                            INT count;
                            INT index;
                            PBUTTON_CONTEXT itemContext;

                            count = ListBox_GetCount(context->CurrentListHandle);
                            index = ListBox_GetCurSel(context->CurrentListHandle);

                            if (count == LB_ERR)
                                break;

                            if (index == LB_ERR)
                                break;

                            itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(context->CurrentListHandle, index);

                            if (itemContext == NULL)
                                break;

                            if (index == 0 && count == 2)
                            {
                                // first and last item
                                Button_Enable(context->MoveUpButtonHandle, FALSE);
                                Button_Enable(context->MoveDownButtonHandle, FALSE);
                            }
                            else if (index == (count - 1))
                            {
                                // last item (virtual separator)
                                Button_Enable(context->MoveUpButtonHandle, FALSE);
                                Button_Enable(context->MoveDownButtonHandle, FALSE);
                            }
                            else if (index == (count - 2))
                            {
                                // second last item (last non-virtual item)
                                Button_Enable(context->MoveUpButtonHandle, TRUE);
                                Button_Enable(context->MoveDownButtonHandle, FALSE);
                            }
                            else if (index == 0)
                            {
                                // first item
                                Button_Enable(context->MoveUpButtonHandle, FALSE);
                                Button_Enable(context->MoveDownButtonHandle, TRUE);
                            }
                            else
                            {
                                Button_Enable(context->MoveUpButtonHandle, TRUE);
                                Button_Enable(context->MoveDownButtonHandle, TRUE);
                            }

                            Button_Enable(context->RemoveButtonHandle, itemContext->IsRemovable);
                        }
                        break;
                    case LBN_DBLCLK:
                        {
                            INT count;
                            INT index;

                            count = ListBox_GetCount(context->CurrentListHandle);
                            index = ListBox_GetCurSel(context->CurrentListHandle);

                            if (count == LB_ERR)
                                break;

                            if (index == LB_ERR)
                                break;

                            if (index == (count - 1))
                            {
                                // virtual separator
                                break;
                            }

                            CustomizeRemoveToolbarItem(context, index);
                        }
                        break;
                    }
                }
                break;
            case IDC_ADD:
                {
                    INT index;
                    INT indexto;

                    index = ListBox_GetCurSel(context->AvailableListHandle);
                    indexto = ListBox_GetCurSel(context->CurrentListHandle);

                    if (index == LB_ERR)
                        break;

                    if (indexto == LB_ERR)
                        break;

                    CustomizeAddToolbarItem(context, index, indexto);
                }
                break;
            case IDC_REMOVE:
                {
                    INT index;

                    index = ListBox_GetCurSel(context->CurrentListHandle);

                    if (index == LB_ERR)
                        break;

                    CustomizeRemoveToolbarItem(context, index);
                }
                break;
            case IDC_MOVEUP:
                {
                    INT index;

                    index = ListBox_GetCurSel(context->CurrentListHandle);

                    if (index == LB_ERR)
                        break;

                    CustomizeMoveToolbarItem(context, index, index - 1);
                }
                break;
            case IDC_MOVEDOWN:
                {
                    INT index;

                    index = ListBox_GetCurSel(context->CurrentListHandle);

                    if (index == LB_ERR)
                        break;

                    CustomizeMoveToolbarItem(context, index, index + 1);
                }
                break;
            case IDC_RESET:
                {
                    // Reset the Toolbar buttons to default settings.
                    ToolbarResetSettings();
                    // Re-load the settings.
                    ToolbarLoadSettings();
                    // Save as the new defaults.
                    ToolbarSaveButtonSettings();

                    CustomizeLoadToolbarItems(context);
                }
                break;
            case IDC_TEXTOPTIONS:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PhSetIntegerSetting(SETTING_NAME_TOOLBARDISPLAYSTYLE,
                            (DisplayStyle = (TOOLBAR_DISPLAY_STYLE)ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam))));

                        ToolbarLoadSettings();
                    }
                }
                break;
            case IDC_SEARCHOPTIONS:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PhSetIntegerSetting(SETTING_NAME_SEARCHBOXDISPLAYMODE,
                            (SearchBoxDisplayMode = (SEARCHBOX_DISPLAY_MODE)ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam))));

                        ToolbarLoadSettings();
                    }
                }
                break;
            case IDC_THEMEOPTIONS:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == CBN_SELCHANGE)
                    {
                        PhSetIntegerSetting(SETTING_NAME_TOOLBAR_THEME,
                            (ToolBarTheme = (TOOLBAR_THEME)ComboBox_GetCurSel(GET_WM_COMMAND_HWND(wParam, lParam))));

                        switch (ToolBarTheme)
                        {
                        case TOOLBAR_THEME_NONE:
                            {
                                SendMessage(RebarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"");
                                SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"");
                            }
                            break;
                        case TOOLBAR_THEME_BLACK:
                            {
                                SendMessage(RebarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Media");
                                SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Media");
                            }
                            break;
                        case TOOLBAR_THEME_BLUE:
                            {
                                SendMessage(RebarHandle, RB_SETWINDOWTHEME, 0, (LPARAM)L"Communications");
                                SendMessage(ToolBarHandle, TB_SETWINDOWTHEME, 0, (LPARAM)L"Communications");
                            }
                            break;
                        }
                    }
                }
                break;
            case IDC_ENABLE_MODERN:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        ToolStatusConfig.ModernIcons = Button_GetCheck(GET_WM_COMMAND_HWND(wParam, lParam)) == BST_CHECKED;

                        PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                        ToolbarLoadSettings();

                        CustomizeResetImages(context);
                        CustomizeResetToolbarImages();
                        //CustomizeLoadItems(context);
                    }
                }
                break;
            case IDC_ENABLE_AUTOHIDE_MENU:
                {
                    if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
                    {
                        ToolStatusConfig.AutoHideMenu = !ToolStatusConfig.AutoHideMenu;

                        PhSetIntegerSetting(SETTING_NAME_TOOLSTATUS_CONFIG, ToolStatusConfig.Flags);

                        if (ToolStatusConfig.AutoHideMenu)
                        {
                            SetMenu(PhMainWndHandle, NULL);
                        }
                        else
                        {
                            SetMenu(PhMainWndHandle, MainMenu);
                            DrawMenuBar(PhMainWndHandle);
                        }
                    }
                }
                break;
            case IDCANCEL:
                {
                    EndDialog(hwndDlg, FALSE);
                }
                break;
            }
        }
        break;
    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT drawInfo = (LPDRAWITEMSTRUCT)lParam;

            if (drawInfo->CtlID == IDC_AVAILABLE || drawInfo->CtlID == IDC_CURRENT)
            {
                HDC bufferDc;
                HBITMAP bufferBitmap;
                HBITMAP oldBufferBitmap;
                PBUTTON_CONTEXT itemContext;
                RECT bufferRect =
                {
                    0, 0,
                    drawInfo->rcItem.right - drawInfo->rcItem.left,
                    drawInfo->rcItem.bottom - drawInfo->rcItem.top
                };
                BOOLEAN isSelected = (drawInfo->itemState & ODS_SELECTED) == ODS_SELECTED;
                BOOLEAN isFocused = (drawInfo->itemState & ODS_FOCUS) == ODS_FOCUS;

                if (drawInfo->itemID == LB_ERR)
                    break;

                itemContext = (PBUTTON_CONTEXT)ListBox_GetItemData(drawInfo->hwndItem, drawInfo->itemID);
                if (itemContext == NULL)
                    break;

                bufferDc = CreateCompatibleDC(drawInfo->hDC);
                bufferBitmap = CreateCompatibleBitmap(drawInfo->hDC, bufferRect.right, bufferRect.bottom);
                oldBufferBitmap = SelectBitmap(bufferDc, bufferBitmap);
                SelectFont(bufferDc, context->FontHandle);

                SetBkMode(bufferDc, TRANSPARENT);
                FillRect(bufferDc, &bufferRect, GetSysColorBrush(isFocused ? COLOR_HIGHLIGHT : COLOR_WINDOW));

                if (isSelected)
                {
                    FrameRect(bufferDc, &bufferRect, isFocused ? GetStockBrush(BLACK_BRUSH) : GetSysColorBrush(COLOR_HIGHLIGHT));
                }
                else
                {
                    FrameRect(bufferDc, &bufferRect, isFocused ? GetStockBrush(BLACK_BRUSH) : GetSysColorBrush(COLOR_HIGHLIGHTTEXT));
                }

                if (itemContext->IsVirtual)
                {
                    SetTextColor(bufferDc, GetSysColor(COLOR_GRAYTEXT));
                }
                else
                {
                    SetTextColor(bufferDc, GetSysColor(isFocused ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));
                }

                if (!itemContext->IsSeparator)
                {
                    ImageList_Draw(
                        context->ImageListHandle,
                        itemContext->IdBitmap,
                        bufferDc,
                        bufferRect.left + 2,
                        bufferRect.top + 2,
                        ILD_NORMAL
                        );
                }

                bufferRect.left += context->BitmapWidth; //+ 2;

                if (itemContext->IdCommand != 0)
                {
                    DrawText(
                        bufferDc,
                        ToolbarGetText(itemContext->IdCommand),
                        -1,
                        &bufferRect,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP
                        );
                }
                else
                {
                    DrawText(
                        bufferDc,
                        L"Separator",
                        -1,
                        &bufferRect,
                        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOCLIP
                        );
                }

                BitBlt(
                    drawInfo->hDC,
                    drawInfo->rcItem.left,
                    drawInfo->rcItem.top,
                    drawInfo->rcItem.right,
                    drawInfo->rcItem.bottom,
                    bufferDc,
                    0,
                    0,
                    SRCCOPY
                    );

                SelectBitmap(bufferDc, oldBufferBitmap);
                DeleteBitmap(bufferBitmap);
                DeleteDC(bufferDc);

                return TRUE;
            }
        }
        break;
    }

    return FALSE;
}

VOID ToolBarShowCustomizeDialog(
    VOID
    )
{
    DialogBox(
        PluginInstance->DllBase,
        MAKEINTRESOURCE(IDD_CUSTOMIZE_TB),
        PhMainWndHandle,
        CustomizeToolbarDialogProc
        );
}