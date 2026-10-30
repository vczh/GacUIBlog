# Edit Word documents through PowerShell COM

Use this document as a reusable instruction prompt and API reference for editing Microsoft Word files on Windows. Operate the desktop application through `Word.Application` from PowerShell. Read the Office VBA reference for object relationships, parameter order, return values, units, and enum values, then translate its calling syntax into PowerShell.

The API facts below were checked against Microsoft's documentation on 2026-10-30. The examples are original PowerShell adaptations. A documentation check or syntax check does not establish that every recipe works on every Word build or document; verify the operations used by the actual task.

All 21 PowerShell blocks were syntax-checked in Windows PowerShell 5.1 and PowerShell 7.6.9. The complete bookmark-copy example was executed with Word 16.0 in both shells, including saving and reopening the output. The REF, PAGE, DATE, story-field update, and TOC update recipes were also exercised in both shells, and the custom-style recipe was exercised in PowerShell 7.6.9. These checks used disposable documents; other recipes and the actual task's document still need appropriate validation.

## Working instructions

1. Establish the input file, requested edits, output file, and whether the task concerns saved content or a live document with unsaved edits. Inspect the relevant ranges, styles, tables, sections, bookmarks, and controls first.
2. Keep an explicit `Document` reference. Prefer `Range` operations to `Selection`, `ActiveDocument`, clipboard state, or keyboard automation. Resolve a target unambiguously before changing it.
3. Work on a copy unless an in-place edit was requested. A filesystem copy contains only the saved file. Do not assume Word has PowerPoint's or Excel's `SaveCopyAs` method: verify a Word-specific way to capture live state before choosing a workflow.
4. Make the smallest change that fulfills the request. Preserve unrelated formatting, fields, revisions, comments, bookmarks, links, controls, section breaks, headers, footers, and macros. Replacing the entire document's plain text is not a preservation strategy.
5. **Separate content from presentation and use named styles by default throughout the work.** Write semantic content, apply paragraph/character/table styles, and maintain formatting centrally in style definitions. Preserve an existing template's styles; reserve direct formatting for narrow, intentional exceptions. Use real paragraphs, tables, lists, and content controls, and let text wrap naturally.
6. Save explicitly, reopen the output, verify the requested content and structure, and inspect a rendered document when an edit can affect layout or pagination.
7. Close only documents opened for the operation. Quit Word only when its application session is known to be dedicated to the script. Do not terminate every `WINWORD` process as cleanup.

Start with the [Word VBA reference](https://learn.microsoft.com/en-us/office/vba/api/overview/word) and [Document object](https://learn.microsoft.com/en-us/office/vba/api/word.document). The automation object model is usable by clients other than VBA.

## Runtime and connection

Use Windows with desktop Word installed and registered for COM. Windows PowerShell 5.1 is a useful compatibility baseline; PowerShell 7 on Windows also provides `New-Object -ComObject`. Run the session in a single-threaded apartment (STA), and keep its COM objects on that thread. Do not pass them into parallel runspaces, jobs, or remoting sessions.

```powershell
powershell.exe -NoProfile -STA -File .\Edit-Document.ps1
# Or, with PowerShell 7 installed on Windows:
pwsh.exe -NoProfile -STA -File .\Edit-Document.ps1
```

```powershell
$ErrorActionPreference = 'Stop'
if ([Threading.Thread]::CurrentThread.ApartmentState -ne 'STA') {
    throw 'Run Word automation in an STA PowerShell process.'
}
$app = New-Object -ComObject Word.Application
$documents = $app.Documents
Get-Member -InputObject $documents -Name Open
$app.Version
```

For an explicitly requested live-session edit, attach to the intended application and identify the document by `FullName`. Windows PowerShell 5.1 can use `[Runtime.InteropServices.Marshal]::GetActiveObject('Word.Application')`; this .NET Framework method is not available as the same built-in call in PowerShell 7. Starting a new automation object is not an instruction to use whatever document happens to be active.

This guide targets desktop editing. Microsoft documents limitations for [unattended, non-interactive Office automation](https://learn.microsoft.com/en-us/office/client-developer/integration/considerations-unattended-automation-office-microsoft-365-for-unattended-rpa). Hiding Word's window does not make it a headless document service.

Sources: [New-Object](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/new-object), [PowerShell COM objects](https://learn.microsoft.com/en-us/powershell/scripting/samples/creating-.net-and-com-objects--new-object-), [Windows PowerShell STA](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_powershell_exe?view=powershell-5.1), [PowerShell 7 STA](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_pwsh).

## Translate VBA to PowerShell

| VBA concept | PowerShell COM form |
| --- | --- |
| `Set app = CreateObject("Word.Application")` | `$app = New-Object -ComObject Word.Application` |
| `Set p = doc.Paragraphs(1)` | `$paragraph = $doc.Paragraphs.Item(1)` |
| `Set r = doc.Range(0, 10)` | `$range = $doc.Range(0, 10)` |
| `r.Text = "Hello"` | `$range.Text = 'Hello'` |
| `doc.Save` | `$doc.Save()` |
| `doc.SaveAs2 FileName:=path, FileFormat:=wdFormatDocumentDefault` | `$doc.SaveAs2($path, 16)` |
| `With obj ... End With` | Assign `$obj`, then qualify each member. |
| Omitted optional argument | Omit trailing arguments, or use `[Type]::Missing` for an intervening optional slot supported by the binder. |
| `Nothing` object argument | Usually `$null`; follow the member's contract. |
| VBA `Long` / `Single` | `[int]` / `[single]` when explicit argument types are needed. |

Use parentheses and positional arguments for methods, not VBA's `Name:=value` syntax. Optional values, null objects, and Boolean false are different. Word's primary interop signatures often expose `ref object` parameters; late-bound COM calls commonly accept ordinary positional values. If the installed binding requires references, use variables and `[ref]$variable` for the applicable parameters. Do not blindly add `[ref]` to every argument or pass `[ref]` to a literal. Inspect `Get-Member` and the actual signature when adapting an early-bound example.

Most Word collections are **1-based**, but Word `Range.Start` and `Range.End` are **0-based character positions**, with `End` immediately after the range. `$doc.Range(0, 10)` covers its first ten character positions. PowerShell arrays are also 0-based, but .NET string indexes are not a substitute for Word positions across arbitrary document structures.

Suppress unused method results with `[void]` or capture them. PowerShell functions output every uncaptured expression. Use indexed collection loops when mutation or explicit COM lifetime matters; avoid `1..$collection.Count` if the count could be zero.

### Constants and value types

Late binding does not import VBA constant names. Define the numeric values used by a script, or use compatible interop enums if those assemblies are already available.

| Constant | Value | Purpose |
| --- | ---: | --- |
| `wdCollapseEnd`, `wdCollapseStart` | `0`, `1` | Collapse a range to an insertion point. |
| `wdFindStop`, `wdFindContinue` | `0`, `1` | Stop at the range boundary, or wrap. |
| `wdReplaceNone`, `wdReplaceOne`, `wdReplaceAll` | `0`, `1`, `2` | Find/replace behavior. |
| `wdDoNotSaveChanges`, `wdSaveChanges`, `wdPromptToSaveChanges` | `0`, `-1`, `-2` | Word document/application close behavior. |
| `wdFormatDocumentDefault`, `wdFormatXMLDocument` | `16`, `12` | DOCX formats; select the intended format explicitly. |
| `wdFormatXMLDocumentMacroEnabled` | `13` | DOCM. |
| `wdFormatXMLTemplate`, `wdFormatXMLTemplateMacroEnabled` | `14`, `15` | DOTX / DOTM. |
| `wdExportFormatPDF` | `17` | PDF export. |
| `wdExportDocumentContent`, `wdExportDocumentWithMarkup` | `0`, `7` | Export without / with markup. |
| `wdStyleNormal`, `wdStyleHeading1`, `wdStyleHeading2` | `-1`, `-2`, `-3` | Built-in styles, independent of localized display names. |
| `wdHeaderFooterPrimary`, `wdHeaderFooterFirstPage`, `wdHeaderFooterEvenPages` | `1`, `2`, `3` | Header/footer variants. |
| `wdNoProtection` | `-1` | No document protection. |
| `msoAutomationSecurityForceDisable` | `3` | Disable macros while opening programmatically. |

Many Word members, such as `TrackRevisions` and `LinkToPrevious`, use Boolean values. Some formatting properties, such as `Font.Bold` and `KeepWithNext`, use a Long supporting true, false, and mixed/undefined values. Do not treat every nonzero mixed result as true. `Close` takes a `WdSaveOptions` value; it does not have Excel's Boolean `SaveChanges` contract.

Sources: [WdCollapseDirection](https://learn.microsoft.com/en-us/office/vba/api/word.wdcollapsedirection), [WdFindWrap](https://learn.microsoft.com/en-us/office/vba/api/word.wdfindwrap), [WdReplace](https://learn.microsoft.com/en-us/office/vba/api/word.wdreplace), [WdSaveOptions](https://learn.microsoft.com/en-us/office/vba/api/word.wdsaveoptions), [WdSaveFormat](https://learn.microsoft.com/en-us/office/vba/api/word.wdsaveformat), [WdExportItem](https://learn.microsoft.com/en-us/office/vba/api/word.wdexportitem), [WdBuiltinStyle](https://learn.microsoft.com/en-us/office/vba/api/word.wdbuiltinstyle), [WdHeaderFooterIndex](https://learn.microsoft.com/en-us/office/vba/api/word.wdheaderfooterindex), [WdProtectionType](https://learn.microsoft.com/en-us/office/vba/api/word.wdprotectiontype).

## Object model and inspection

```text
Application
  Documents.Item(...) -> Document
    Content / Range(start, end) -> Range in the main story
    StoryRanges -> story roots -> NextStoryRange
    Paragraphs.Item(...).Range -> text, Style, Font, ParagraphFormat
    Bookmarks.Item(name).Range
    ContentControls.Item(...).Range
    Tables.Item(...).Cell(row, column).Range
    Sections.Item(...)
      PageSetup
      Headers.Item(type).Range / Footers.Item(type).Range
    InlineShapes / Shapes -> pictures and floating objects
    Fields / TablesOfContents / Revisions / Comments
```

Inspect before targeting an ordinal paragraph or table:

```powershell
$paragraphs = $doc.Paragraphs
for ($i = 1; $i -le $paragraphs.Count; $i++) {
    $paragraph = $paragraphs.Item($i)
    $range = $paragraph.Range
    [pscustomobject]@{
        Index = $i
        Start = $range.Start
        End = $range.End
        Style = $range.Style.NameLocal
        Text = $range.Text
    }
}
```

`Document.Content` covers the main text story. It is not all visible text in the file. Headers, footers, notes, comments, and text boxes can live in other stories. For inspection, traverse the available story roots and their chains:

```powershell
foreach ($rootStory in $doc.StoryRanges) {
    $story = $rootStory
    while ($null -ne $story) {
        [pscustomobject]@{ StoryType = $story.StoryType; Text = $story.Text }
        $story = $story.NextStoryRange
    }
}
```

This traversal is a starting point, not a guarantee that every embedded object or nested shape's text was covered. Inspect floating text boxes and linked headers explicitly when relevant. Do not perform repeated writes to the same linked header merely because it is reachable from several sections. [Range object](https://learn.microsoft.com/en-us/office/vba/api/word.range), [NextStoryRange](https://learn.microsoft.com/en-us/office/vba/api/word.range.nextstoryrange).

## Complete example: update a bookmark in a copy

Save this block as `Edit-Document.ps1`. It changes a named plain-text bookmark in a saved DOCX/DOCM copy, recreates the bookmark, saves, and reopens the output to verify the result. It expects an ordinary main-story bookmark without paragraph marks, fields, tables, or inline images. It rejects revision-bearing or protected input rather than silently choosing review semantics. Use a different, explicit policy when the task requires those features.

```powershell
param(
    [Parameter(Mandatory)][string]$InputPath,
    [Parameter(Mandatory)][string]$OutputPath,
    [Parameter(Mandatory)][ValidateNotNullOrEmpty()][string]$BookmarkName,
    [Parameter(Mandatory)][AllowEmptyString()][string]$NewText
)
$ErrorActionPreference = 'Stop'
if ([Threading.Thread]::CurrentThread.ApartmentState -ne 'STA') {
    throw 'Run this script with -STA on Windows.'
}
if ($NewText -match '[\r\n\a]') { throw 'This example expects inline text.' }
$source = (Resolve-Path -LiteralPath $InputPath).ProviderPath
$destination = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
$extension = [IO.Path]::GetExtension($source).ToLowerInvariant()
if ($extension -notin @('.docx', '.docm')) { throw 'Expected a DOCX or DOCM file.' }
if ([IO.Path]::GetExtension($destination).ToLowerInvariant() -ne $extension) {
    throw 'Keep the original extension in this example.'
}
if ($source -ieq $destination -or (Test-Path -LiteralPath $destination)) {
    throw 'Choose a new output path distinct from the input.'
}
if (-not (Test-Path -LiteralPath (Split-Path -Parent $destination) -PathType Container)) {
    throw 'Create the output directory first.'
}
[IO.File]::Copy($source, $destination, $false)
$app = $documents = $doc = $check = $bookmarks = $range = $null
try {
    $app = New-Object -ComObject Word.Application
    $documents = $app.Documents
    $oldSecurity = $app.AutomationSecurity
    try {
        $app.AutomationSecurity = 3
        # Open(FileName, ConfirmConversions, ReadOnly, AddToRecentFiles)
        $doc = $documents.Open($destination, $false, $false, $false)
    }
    finally { $app.AutomationSecurity = $oldSecurity }
    if ($doc.ReadOnly -or $doc.ProtectionType -ne -1) {
        throw 'The working document is not freely editable.'
    }
    if ($doc.TrackRevisions -or $doc.Revisions.Count -gt 0) {
        throw 'Choose an explicit tracked-changes policy for this document.'
    }
    $bookmarks = $doc.Bookmarks
    if (-not $bookmarks.Exists($BookmarkName)) { throw 'Bookmark not found.' }
    $range = $bookmarks.Item($BookmarkName).Range
    if ($range.StoryType -ne 1 -or $range.Text -match '[\r\n\a]' -or
        $range.Fields.Count -gt 0 -or $range.Tables.Count -gt 0 -or
        $range.InlineShapes.Count -gt 0) {
        throw 'Bookmark is outside this example''s plain-text scope.'
    }
    $range.Text = $NewText
    [void]$bookmarks.Add($BookmarkName, $range)
    $doc.Save()
    $range = $bookmarks = $null
    $doc.Close(0)
    $doc = $null
    try {
        $app.AutomationSecurity = 3
        $check = $documents.Open($destination, $false, $true, $false)
    }
    finally { $app.AutomationSecurity = $oldSecurity }
    if (-not $check.Bookmarks.Exists($BookmarkName)) {
        throw 'The saved bookmark is missing.'
    }
    if ([string]$check.Bookmarks.Item($BookmarkName).Range.Text -cne $NewText) {
        throw 'Saved bookmark text does not match.'
    }
    if ((Get-Item -LiteralPath $destination).Length -eq 0) { throw 'Output is empty.' }
    Write-Output "Verified output: $destination"
}
finally {
    $range = $bookmarks = $null
    try { if ($null -ne $check) { $check.Close(0) } }
    finally {
        if ($null -ne $doc) { $doc.Close(0) }
        $check = $doc = $documents = $app = $null
    }
}
```

On failure, the script discards unsaved changes to its own copy. An existing output file may just be the original copied bytes; report success only after verification. The script leaves Word running. In a known dedicated application session, quit after closing its documents; do not apply that policy to a user's attached session. A binding that requires a reference accepts `$noSave = 0` followed by `$app.Quit([ref]$noSave)`; this form was exercised in both shells. An ordinary `$app.Quit(0)` can fail with a `PSReference` error in Windows PowerShell 5.1.

`Documents.Open` has many optional parameters. Its `Revert` argument can discard unsaved edits in an already open document; do not enable it as a generic retry. `ReadOnly=$false` does not override a document's read-only recommendation or protection. Save the old `AutomationSecurity`, set it immediately around opening, and restore it in `finally`. COM editing does not require enabling document macros.

Sources: [Documents.Open](https://learn.microsoft.com/en-us/office/vba/api/word.documents.open), [AutomationSecurity](https://learn.microsoft.com/en-us/office/vba/api/word.application.automationsecurity), [Bookmarks.Add](https://learn.microsoft.com/en-us/office/vba/api/word.bookmarks.add), [Document.Close](https://learn.microsoft.com/en-us/office/vba/api/Word.Document.Close%28method%29).

## Ranges, text, and insertion points

A `Range` is independent of the user's selection. Use `.Duplicate` when a search or boundary adjustment should not redefine the caller's range. `Range.Text` reads plain text and replaces the range's existing content when assigned; it is not a rich-text round trip. Prefer a small range change, or use `FormattedText` when copying intentionally formatted content.

```powershell
# Replace paragraph text while retaining its final paragraph mark.
$range = $doc.Paragraphs.Item(3).Range.Duplicate
if ($range.Text.EndsWith("`r")) { $range.End = $range.End - 1 }
$range.Text = 'Updated paragraph text.'

# Insert before the document's final paragraph mark.
$position = $doc.Content.End - 1
$insertion = $doc.Range($position, $position)
$insertion.InsertBefore('Additional text.')
```

Paragraph marks carry paragraph formatting and delimit document structure. Do not remove them while making an inline correction. Use `` `r `` for an intentional paragraph break; natural visual wrapping requires no newline. Collapsing a whole paragraph to its end places the insertion point after its paragraph mark, potentially in the next paragraph. Range coordinates and collection indexes change after edits: reacquire targets or work backward through inspected positions.

Sources: [Range.Text](https://learn.microsoft.com/en-us/office/vba/api/word.range.text), [Range.Collapse](https://learn.microsoft.com/en-us/office/vba/api/word.range.collapse), [Range.FormattedText](https://learn.microsoft.com/en-us/office/vba/api/word.range.formattedtext).

## Find and replace

Word Find is stateful. Set options explicitly instead of inheriting the last interactive search. Clear search and replacement formatting separately. Use `Wrap=0` (`wdFindStop`) to keep a search inside the inspected range. A successful range-based find redefines that range to the match; it does not move the selection.

This example replaces an ordinary Latin-text token in the main story. All replacements are authorized here; use `Replace=0` and inspect matches first when scope or counts matter.

```powershell
$search = $doc.Content.Duplicate
$find = $search.Find
$find.ClearFormatting()
$find.Replacement.ClearFormatting()
$find.MatchPrefix = $false
$find.MatchSuffix = $false
$find.MatchPhrase = $false
$find.IgnoreSpace = $false
$find.IgnorePunct = $false
# FindText, MatchCase, MatchWholeWord, MatchWildcards,
# MatchSoundsLike, MatchAllWordForms, Forward, Wrap, Format,
# ReplaceWith, Replace
$found = $find.Execute(
    'Project Falcon', $true, $false, $false,
    $false, $false, $true, 0, $false,
    'Project Heron', 2
)
```

`Execute` returns a Boolean, not a replacement count. The argument list also includes language-dependent matching options such as Kashida, diacritics, Alef-Hamza, and bidirectional control characters. Configure those explicitly when applicable; availability depends on installed language support. Word wildcards are not .NET regular expressions. Even with wildcards disabled, Word interprets special search/replacement codes such as `^p` and `^t`; do not describe arbitrary input to this recipe as a literal-string replacement engine. `MatchCase=$false` can also change replacement casing for uppercase matches.

For a match-by-match edit, search a duplicate range, record the match's boundaries, edit it, then resume after the replacement within a correctly adjusted remaining range. Do not repeatedly search the same unchanged start when replacement text can contain the search term. A main-story search does not cover headers or text boxes.

Sources: [Find.Execute](https://learn.microsoft.com/en-us/office/vba/api/word.find.execute), [Range.Find](https://learn.microsoft.com/en-us/office/vba/api/word.range.find), [MatchPhrase](https://learn.microsoft.com/en-us/office/vba/api/word.find.matchphrase), [IgnorePunct](https://learn.microsoft.com/en-us/office/vba/api/word.find.ignorepunct).

## Separate content from styles by default

Use this approach for every document: content expresses meaning, named styles express its presentation, and ranges connect them. An ordinary wording change should retain its existing style. When designing a document, define its styles centrally and apply them to semantic roles rather than setting `Font` and `ParagraphFormat` independently on each passage. A later presentation change can then update a style once instead of searching for visually similar text.

| Role | Style choice |
| --- | --- |
| Title, heading, body, caption | Paragraph style, applied to the intended paragraph. |
| Emphasis or an inline code term | Character style, applied to that text range. |
| Header row, bands, borders | Table style and its relevant table-style options. |
| Numbered or bulleted hierarchy | Existing list style/template and level, coordinated with paragraph styles. |

Inspect and reuse the document's or template's styles before adding new ones. Built-in numeric IDs such as `wdStyleNormal=-1` and `wdStyleHeading1=-2` avoid localized display names. Custom styles are named by the template; resolve those names explicitly. Changing a definition affects all content using it, including styles that inherit from it, so a local correction is not a reason to redefine `Normal` throughout an established document. [Range.Style](https://learn.microsoft.com/en-us/office/vba/api/word.range.style), [WdBuiltinStyle](https://learn.microsoft.com/en-us/office/vba/api/word.wdbuiltinstyle).

This recipe defines or updates two document-specific paragraph styles, then applies them to inspected paragraphs 2 and 3. Use it when those style changes and target paragraphs are part of the task; the document must contain at least three paragraphs. The example deliberately keeps presentation in the first part and content in the second.

```powershell
function Get-OrCreateParagraphStyle {
    param($Document, [string]$Name)
    $styles = $Document.Styles
    for ($i = 1; $i -le $styles.Count; $i++) {
        $candidate = $styles.Item($i)
        if ($candidate.NameLocal -eq $Name) {
            if ($candidate.Type -ne 1) { throw "Style '$Name' is not a paragraph style." }
            return $candidate
        }
    }
    return $styles.Add($Name, 1)  # wdStyleTypeParagraph
}

# Presentation definitions. Choose values that fit the document's design.
$bodyStyle = Get-OrCreateParagraphStyle $doc 'Report Body'
$bodyStyle.BaseStyle = -1  # wdStyleNormal
$bodyStyle.AutomaticallyUpdate = $false
$bodyStyle.Font.Size = 11
$bodyStyle.ParagraphFormat.SpaceAfter = 8

$headingStyle = Get-OrCreateParagraphStyle $doc 'Report Heading'
$headingStyle.BaseStyle = -2  # wdStyleHeading1; retains heading semantics
$headingStyle.AutomaticallyUpdate = $false
$headingStyle.Font.Size = 16
$headingStyle.ParagraphFormat.KeepWithNext = -1
$headingStyle.ParagraphFormat.SpaceBefore = 12
$headingStyle.ParagraphFormat.SpaceAfter = 6
$headingStyle.NextParagraphStyle = $bodyStyle

# Content and role assignments. Preserve the paragraph marks.
$headingText = $doc.Paragraphs.Item(2).Range.Duplicate
$headingText.End = $headingText.End - 1
$headingText.Text = 'Project status'
$doc.Paragraphs.Item(2).Range.Style = $headingStyle
$bodyText = $doc.Paragraphs.Item(3).Range.Duplicate
$bodyText.End = $bodyText.End - 1
$bodyText.Text = 'The first review is complete.'
$doc.Paragraphs.Item(3).Range.Style = $bodyStyle
```

`Styles.Add(Name, Type)` uses `1` for paragraph styles, `2` for character styles, `3` for table styles, and `4` for list styles. `BaseStyle` supplies inheritance; set only intentional differences from the base. Keep `AutomaticallyUpdate=$false` on these paragraph styles so an incidental paragraph-format change does not redefine the style. This property concerns paragraph styles; it is not an instruction to alter every style type or the attached template. [Styles.Add](https://learn.microsoft.com/en-us/office/vba/api/word.styles.add), [WdStyleType](https://learn.microsoft.com/en-us/office/vba/api/word.wdstyletype), [BaseStyle](https://learn.microsoft.com/en-us/office/vba/api/word.style.basestyle), [AutomaticallyUpdate](https://learn.microsoft.com/en-us/office/vba/api/word.style.automaticallyupdate), [NextParagraphStyle](https://learn.microsoft.com/en-us/office/vba/api/word.style.nextparagraphstyle).

Use a character style such as `$emphasisRange.Style = -89` (`wdStyleEmphasis`) for semantic emphasis. A heading must remain a heading, not merely large bold body text: preserve its outline level, numbering relationships, and the table of contents' style/outline settings. A custom style based on Heading 1 should be verified against the document's actual TOC configuration. `Range.Style` on a mixed-style range reports only its first character or paragraph style, so inspect paragraphs/runs separately when inventorying styles.

Direct formatting can override styles and obscure their effects. Do not automatically erase existing overrides, hyperlinks, or emphasis with a whole-document formatting reset. Use direct formatting only for a deliberate exception, and use a named style when the role repeats. Style definitions set within the document do not require editing `Normal.dotm` or saving changes to an attached template.

Paragraph spacing, indents, font sizes, and page margins use points, with 72 points per inch. Set the intended `LineSpacingRule` in the style before `LineSpacing`; interpretation depends on the rule. Avoid spaces and tabs as substitutes for indents or columns. For lists, preserve the template and level; `ListFormat` operations can restart numbering or attach paragraphs to another list. [ParagraphFormat](https://learn.microsoft.com/en-us/office/vba/api/word.paragraphformat), [ListFormat](https://learn.microsoft.com/en-us/office/vba/api/word.listformat).

## Bookmarks and content controls

Replacing a nonempty bookmark's `Range.Text` can remove the bookmark. Retain its range and recreate the bookmark after the edit, as in the complete example. Bookmark names are limited to 40 characters and cannot contain spaces. Replacing all text in a bookmark may remove nested objects or other bookmarks; inspect the target rather than treating every bookmark as a plain-text field.

For a template with content controls, target an inspected tag and require the expected match count:

```powershell
$controls = $doc.SelectContentControlsByTag('CustomerName')
if ($controls.Count -ne 1) { throw 'Expected exactly one CustomerName control.' }
$control = $controls.Item(1)
if ($control.LockContents) { throw 'The content control is locked.' }
# This recipe expects a plain-text or suitable rich-text content control.
$control.Range.Text = 'Example Customer'
```

A tag is not necessarily unique. Check the control type, nesting, locks, placeholder state, and XML mapping. Dropdowns, dates, checkboxes, pictures, and repeating sections need their own APIs. A mapped control may update other controls bound to the same data. Do not remove locks, mappings, or document protection just to force a generic text assignment.

Sources: [Bookmarks.Add](https://learn.microsoft.com/en-us/office/vba/api/word.bookmarks.add), [Bookmark behavior](https://learn.microsoft.com/en-us/visualstudio/vsto/bookmark-control), [SelectContentControlsByTag](https://learn.microsoft.com/en-us/office/vba/api/word.document.selectcontentcontrolsbytag), [ContentControl.Range](https://learn.microsoft.com/en-us/office/vba/api/word.contentcontrol.range).

## Tables and cell boundaries

`Tables.Add(Range, NumRows, NumColumns, DefaultTableBehavior, AutoFitBehavior)` returns a `Table`. A noncollapsed insertion range is replaced, so collapse the range unless replacement is intended. Table row and column indexes start at 1.

```powershell
$position = $doc.Content.End - 1
$insertion = $doc.Range($position, $position)
$table = $doc.Tables.Add($insertion, 3, 2)
$table.Cell(1, 1).Range.Text = 'Item'
$table.Cell(1, 2).Range.Text = 'Amount'
$table.Cell(2, 1).Range.Text = 'Service'
$table.Cell(2, 2).Range.Text = '1250'
$table.Style = -159  # wdStyleTableLightShading; use the template's style when present
$table.ApplyStyleHeadingRows = $true
```

Word table cells contain structural terminators. A cell's `.Range.Text` commonly ends in carriage return plus character 7, so extracting cell data requires removing that terminal pair without globally stripping meaningful whitespace. A focused content-range edit can exclude the final cell marker:

```powershell
$cellRange = $table.Cell(2, 1).Range.Duplicate
$cellRange.End = $cellRange.End - 1
$cellRange.Text = 'Consulting service'
```

Test cell-boundary handling on the actual structure, especially with nested tables or revisions. Do not rebuild a table from tab-separated plain text when preserving cell styling matters. Merged cells can make rectangular indexing and `Rows`/`Columns` access unsuitable; inspect the cell ranges actually present. Row heights, split-across-pages behavior, repeating heading rows, and AutoFit affect pagination and need rendering checks.

The selected table style must define header-row formatting for `ApplyStyleHeadingRows` to have that effect. This setting is separate from repeating heading rows on later pages.

Sources: [Tables.Add](https://learn.microsoft.com/en-us/office/vba/api/word.tables.add), [Cell.Range](https://learn.microsoft.com/en-us/office/vba/api/word.cell.range), [Range.End](https://learn.microsoft.com/en-us/office/vba/api/word.range.end), [Table.Style](https://learn.microsoft.com/en-us/office/vba/api/word.table.style), [ApplyStyleHeadingRows](https://learn.microsoft.com/en-us/office/vba/api/word.table.applystyleheadingrows).

## Sections, headers, footers, and page setup

Page geometry and header/footer relationships belong to sections. Inspect the section count and each section's break type before changing page size or orientation. Deleting a section break can change formatting in adjacent content.

```powershell
$section = $doc.Sections.Item(1)
$setup = $section.PageSetup
$setup.TopMargin = $app.InchesToPoints(0.75)
$setup.BottomMargin = $app.InchesToPoints(0.75)

# Inspect before editing: this may be shared with other sections.
$header = $section.Headers.Item(1)  # wdHeaderFooterPrimary
[pscustomobject]@{ Exists = $header.Exists; Linked = $header.LinkToPrevious; Text = $header.Range.Text }
```

`LinkToPrevious` is independent for each header/footer variant and normally starts true. A change to a linked header may affect several sections. If an explicitly requested section-specific header needs to be independent, set that header's `LinkToPrevious=$false` before editing its existing range; preserve the other variants. Also inspect `DifferentFirstPageHeaderFooter` and `OddAndEvenPagesHeaderFooter` to determine what appears on each page. Do not replace a whole footer range if it contains page-number fields or other content that must remain.

Sources: [PageSetup](https://learn.microsoft.com/en-us/office/vba/api/word.pagesetup), [HeaderFooter.LinkToPrevious](https://learn.microsoft.com/en-us/office/vba/api/word.headerfooter.linktoprevious), [WdHeaderFooterIndex](https://learn.microsoft.com/en-us/office/vba/api/word.wdheaderfooterindex).

## Pictures and floating shapes

Use an inline picture when it should participate in text flow. Use a floating `Shape` only when its anchor, wrap mode, and positioning are part of the intended layout.

```powershell
$imagePath = (Resolve-Path -LiteralPath 'C:\Work\diagram.png').ProviderPath
$anchor = $doc.Bookmarks.Item('DiagramPosition').Range.Duplicate
$anchor.Collapse(1)  # wdCollapseStart: insert without replacing bookmark content
$picture = $doc.InlineShapes.AddPicture($imagePath, $false, $true, $anchor)
$picture.LockAspectRatio = -1
$picture.Width = $app.InchesToPoints(4)
$picture.AlternativeText = 'Diagram of the proposed workflow'
```

`InlineShapes.AddPicture` uses Boolean link/save arguments, unlike PowerPoint's similarly named method with `MsoTriState` arguments. A noncollapsed range is replaced. For floating objects, inspect `Shape.Anchor`, relative horizontal/vertical position, wrapping, and crop before changing them. Merely setting `Left` and `Top` is not enough to establish where a floating image appears on the page.

Sources: [InlineShapes.AddPicture](https://learn.microsoft.com/en-us/office/vba/api/word.inlineshapes.addpicture), [Shape.Anchor](https://learn.microsoft.com/en-us/office/vba/api/word.shape.anchor), [WrapFormat](https://learn.microsoft.com/en-us/office/vba/api/word.wrapformat).

## Fields, tables of contents, and pagination

Fields are Word objects with instructions and generated results. Typical uses include page numbers, cross-references, dates, document properties, sequences, and tables of contents. Typing `{ PAGE }` into `Range.Text` produces ordinary characters, not a working field. Create real fields through `Fields.Add`, or preserve and edit existing `Field` objects.

| Member | Meaning |
| --- | --- |
| `Field.Type` | The `WdFieldType` numeric identifier. |
| `Field.Code` | A `Range` containing the instruction text inside the field delimiters. |
| `Field.Result` | A `Range` containing the currently generated result. |
| `Field.Locked` | A Boolean controlling whether its result can be updated. |
| `Field.Update()` | Recalculate this field; returns Boolean success. |
| `Field.Unlink()` | Replace the field with its current result; removes its dynamic behavior. |

`Code.Text` includes spaces adjacent to the delimiters. Code and result ranges remain available regardless of whether Word is displaying field codes or results. Changing `Result.Text` edits the displayed result, which a later update can overwrite; change the underlying source or field instruction instead. Never automatically unlink fields to simplify plain-text editing. Some field types cannot be unlinked. [Field.Code](https://learn.microsoft.com/en-us/office/vba/api/word.field.code), [Field.Result](https://learn.microsoft.com/en-us/office/vba/api/word.field.result), [Field.Unlink](https://learn.microsoft.com/en-us/office/vba/api/word.field.unlink).

Inspect fields before changing them. This inventory covers the main story; use the story traversal below for other locations.

```powershell
$fields = $doc.Fields
for ($i = 1; $i -le $fields.Count; $i++) {
    $field = $fields.Item($i)
    [pscustomobject]@{
        Index = $i
        Type = [int]$field.Type
        Code = [string]$field.Code.Text
        Result = [string]$field.Result.Text
        Locked = [bool]$field.Locked
    }
}
```

### Create fields and edit their instructions

The VBA contract is `Fields.Add(Range, Type, Text, PreserveFormatting)`, returning a `Field`. A noncollapsed `Range` is replaced. `Type` selects a field kind, while `Text` supplies its arguments and switches; with `wdFieldEmpty=-1`, supply the entire instruction such as `'REF TargetText \h'`. `PreserveFormatting=$true` requests preservation of formatting through updates. This supplements the paragraph/character style applied to its location; it is not a replacement for the document's style system. [Fields.Add](https://learn.microsoft.com/en-us/office/vba/api/word.fields.add).

| Field constant | Value | Example use |
| --- | ---: | --- |
| `wdFieldRef` | `3` | Refer to a bookmark. |
| `wdFieldNumPages` | `26` | Total document pages. |
| `wdFieldDate` | `31` | Date that can change on later updates. |
| `wdFieldPage` | `33` | Page number at the field's location. |
| `wdFieldDocProperty` | `85` | A document property. |

Values: [WdFieldType](https://learn.microsoft.com/en-us/office/vba/api/word.wdfieldtype). Choose a static text date when the value must never change; `DATE`, `CREATEDATE`, and `SAVEDATE` have different meanings.

These recipes create fields in different locations. The first creates a bookmark reference at the end of the main story. The named bookmark must already exist and have the intended source content.

```powershell
if (-not $doc.Bookmarks.Exists('TargetText')) { throw 'TargetText bookmark is missing.' }
$position = $doc.Content.End - 1
$anchor = $doc.Range($position, $position)
$referenceField = $doc.Fields.Add($anchor, 3, 'TargetText \h', $true)
if (-not $referenceField.Update()) { throw 'Could not update the REF field.' }
$referenceField.Code.Text
$referenceField.Result.Text
```

This inserts `PAGE` at the start of the first section's primary footer, retaining its existing content. First inspect that footer, its style, first/even-page variants, and links to other sections; use this recipe only when adding this page number is intended.

```powershell
$pageAnchor = $doc.Sections.Item(1).Footers.Item(1).Range.Duplicate
$pageAnchor.Collapse(1)
$pageField = $pageAnchor.Fields.Add($pageAnchor, 33, '', $true)
if (-not $pageField.Update()) { throw 'Could not update the PAGE field.' }
```

For a date field, PowerShell single-quoted strings preserve the field switch and embedded double quotes. Do not use PowerShell escaping rules as field-code escaping rules.

```powershell
$position = $doc.Content.End - 1
$dateAnchor = $doc.Range($position, $position)
$dateField = $doc.Fields.Add($dateAnchor, 31, '\@ "yyyy-MM-dd"', $true)
if (-not $dateField.Update()) { throw 'Could not update the DATE field.' }
# Change the instruction, then explicitly refresh its result.
$dateField.Code.Text = ' DATE \@ "MMMM d, yyyy" '
if (-not $dateField.Update()) { throw 'Could not refresh the changed DATE field.' }
```

### Update the intended fields in every relevant story

The main document's `Fields` collection does not cover all headers, footers, or text boxes. Traverse `StoryRanges` and each `NextStoryRange` chain to reach relevant story fields. Decide which updates are part of the task: updating dates or external `LINK`, `INCLUDETEXT`, `INCLUDEPICTURE`, or other linked/nested fields can change content beyond the edit. Preserve link instructions and sources; do not blanket-update unknown fields or sever their links.

This example updates inspected page-number and page-count fields across available stories after pagination. Extend the allowed types only for a deliberate update policy. It preserves locked and nested fields for separate review.

```powershell
$doc.Repaginate()
$approvedTypes = @(26, 33)  # wdFieldNumPages, wdFieldPage
foreach ($rootStory in $doc.StoryRanges) {
    $story = $rootStory
    while ($null -ne $story) {
        $storyFields = $story.Fields
        for ($i = 1; $i -le $storyFields.Count; $i++) {
            $field = $storyFields.Item($i)
            if ([int]$field.Type -notin $approvedTypes) { continue }
            if ($field.Locked -or $field.Code.Fields.Count -gt 0) { continue }
            if (-not $field.Update()) {
                throw "Field update failed in story type $($story.StoryType), index $i."
            }
        }
        $story = $story.NextStoryRange
    }
}
```

Inspect floating shapes/text frames separately when required; story traversal is not a guarantee that every embedded object's internal fields were reached. Linked section headers can expose shared content more than once. For complex or nested fields, inspect dependencies and update in an appropriate order instead of assuming a flat collection handles the intended semantics.

`Field.Update()` returns Boolean success, whereas **`Fields.Update()` returns a Long: zero on success, otherwise the index of the first failing field**. Call the collection method only when every field in that range is intended to update. Microsoft notes that its returned error index can be inaccurate on some Word versions. In either case inspect results for stale values and field errors, including missing bookmark references. A locked field is intentionally excluded from updates; report that state instead of silently unlocking it. [Field.Update](https://learn.microsoft.com/en-us/office/vba/api/word.field.update), [Fields.Update](https://learn.microsoft.com/en-us/office/vba/api/word.fields.update), [Field.Locked](https://learn.microsoft.com/en-us/office/vba/api/word.field.locked), [NextStoryRange](https://learn.microsoft.com/en-us/office/vba/api/word.range.nextstoryrange).

### Tables of contents and final pagination

Field results may be cached or stale after editing. `Repaginate()` computes layout; it is not a promise that every field result or TOC entry was refreshed. TOCs depend on heading styles, outline levels, or configured custom styles/entries. Update an intended TOC through its API, then recalculate page numbers after the layout changes.

```powershell
$doc.Repaginate()
if ($doc.TablesOfContents.Count -gt 0) {
    $toc = $doc.TablesOfContents.Item(1)
    $toc.Update()             # Entries and their text
    $doc.Repaginate()
    $toc.UpdatePageNumbers()  # Refresh page numbers after layout changes
}
```

`Update()` refreshes TOC entries, while `UpdatePageNumbers()` refreshes their page references. Inspect every intended TOC and honor its field's locked state before updating. Validate the final rendered pagination and saved field results; a successful update does not establish that the source styles or references were correct. [TableOfContents.Update](https://learn.microsoft.com/en-us/office/vba/api/word.tableofcontents.update), [UpdatePageNumbers](https://learn.microsoft.com/en-us/office/vba/api/word.tableofcontents.updatepagenumbers), [Document.Repaginate](https://learn.microsoft.com/en-us/office/vba/api/word.document.repaginate).

## Tracked changes and comments

Read `TrackRevisions`, the existing revisions, and document protection before an edit. Preserve the requested review workflow. Turning tracking off does not accept existing changes; hiding markup does not remove revisions. Never accept/reject all revisions or delete all comments merely to simplify editing.

```powershell
# Use only when the requested edit should be recorded as a revision.
$oldTrackRevisions = $doc.TrackRevisions
try {
    $doc.TrackRevisions = $true
    $targetRange.Text = 'Proposed replacement text'
}
finally { $doc.TrackRevisions = $oldTrackRevisions }

# Add a comment only when commenting is part of the requested task.
$comment = $doc.Comments.Add($targetRange, 'Please confirm this value.')
```

For modern comments, replies, and resolved state, inspect the installed Word API and verify the result in the actual document. When exporting, choose whether markup belongs in the PDF instead of silently changing the document's review state.

Sources: [TrackRevisions](https://learn.microsoft.com/en-us/office/vba/api/word.document.trackrevisions), [Comments.Add](https://learn.microsoft.com/en-us/office/vba/api/word.comments.add), [WdExportItem](https://learn.microsoft.com/en-us/office/vba/api/word.wdexportitem).

## Save, PDF export, and COM lifetime

| Operation | PowerShell form | Meaning |
| --- | --- | --- |
| Save existing file | `$doc.Save()` | Persist changes to its current file. |
| Save DOCX under a new name | `$doc.SaveAs2($docxPath, 16)` | Save as DOCX and change the document's file identity. |
| Preserve macro-enabled output | `$doc.SaveAs2($docmPath, 13)` | Save as DOCM. |
| Export PDF | `$doc.ExportAsFixedFormat($pdfPath, 17, $false)` | Produce PDF without opening it afterward. |
| Close without another save | `$doc.Close(0)` | Discard pending unsaved edits; save first when retaining changes. |
| Close and save | `$doc.Close(-1)` | Request saving during close; inspect failures and destination. |

Use absolute paths and verify output collisions before `SaveAs2`: Microsoft documents that an existing destination can be overwritten without prompting. Do not change DOCM/DOTM into a nonmacro format if macros must survive. The optional `CompatibilityMode` argument defaults to preserving the current mode; changing the filename alone is not a compatibility upgrade.

For an explicit print-quality PDF of all document content without review markup:

```powershell
# OutputFileName, ExportFormat, OpenAfterExport, OptimizeFor,
# Range, From, To, Item
$doc.ExportAsFixedFormat($pdfPath, 17, $false, 0, 0, 1, 1, 0)
```

Here the page numbers are ignored because `Range=0` exports the entire document. Use the appropriate range and `Item=7` when the requested PDF should include markup. Export is additional output; save the editable Word document separately. Preserve applicable labels and document accessibility settings instead of changing them to make export convenient.

Sources: [SaveAs2](https://learn.microsoft.com/en-us/office/vba/api/word.saveas2), [ExportAsFixedFormat](https://learn.microsoft.com/en-us/office/vba/api/word.document.exportasfixedformat), [Document.Close](https://learn.microsoft.com/en-us/office/vba/api/Word.Document.Close%28method%29).

Save and close owned documents explicitly in `try/finally`. Restore any application settings changed by the script, such as `AutomationSecurity`, `ScreenUpdating`, or `DisplayAlerts`. Do not suppress every alert to conceal a failed save. Assigning `Document.Saved=$true` marks a state; it is not a disk write.

Clear child references when finished. `Application.Quit` closes Word; pass `wdDoNotSaveChanges=0` with the binding syntax described above only after explicitly handling the owned documents. Clearing a variable or releasing a COM wrapper does not mean the application was closed. If deterministic wrapper release is necessary, track exclusively owned objects and release children before parents. Never use released objects or repeatedly call `FinalReleaseComObject` on shared wrappers. Garbage collection does not replace saving, closing, or ownership tracking. [Application.Quit](https://learn.microsoft.com/en-us/office/vba/api/word.application.quit%28method%29), [Marshal.ReleaseComObject](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.interopservices.marshal.releasecomobject).

## Verification and troubleshooting

Reopen the saved file and verify exact requested values, bookmarks/control counts, table structure, styles, section count, and the intended revision state. Text extraction alone does not establish layout fidelity. Render affected pages and inspect wrapping, pagination, headings, page numbers, header/footer variants, tables, image anchors, font substitution, and accidental blank pages. A page count can legitimately change after text edits; compare it against the task's intended outcome rather than requiring it to stay fixed automatically.

| Symptom | Check or response |
| --- | --- |
| Class not registered | Confirm desktop Word and COM registration; an interop assembly alone does not install Word. |
| Wrong parameter type / missing argument | Inspect the installed signature, optional slots, enum family, and any required by-reference variables. |
| Find changes too much or misses text | Reset search options and formatting; check story scope, special codes, and `Wrap`. |
| Bookmark disappears | Retain its edited range and recreate the bookmark intentionally. |
| Text replacement destroys layout | Check whether the range included paragraph marks, fields, objects, table markers, or section breaks. |
| Header changes several sections | Inspect the relevant `LinkToPrevious` chain and header/footer variant. |
| Table cell data contains unexpected characters | Account for cell/paragraph terminators without removing document structure. |
| Save or open fails | Check paths, permissions, locks, protection, Protected View, download completion, and format. |
| Word is busy or rejects a call | Inspect modal dialogs; retry only known rejected calls with a bound, and inspect state before repeating a mutation. |
| Word remains running | Check document and wrapper ownership; leave shared sessions open instead of killing all processes. |

Report documentation review, syntax validation, actual COM execution, save/reopen checks, and visual inspection accurately. A successful call or an existing output file alone does not establish a correct document.
