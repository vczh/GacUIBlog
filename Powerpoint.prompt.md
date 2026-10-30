# Edit PowerPoint presentations through PowerShell COM

Use this document as a reusable instruction prompt and API reference when editing PowerPoint files on Windows. Operate Microsoft PowerPoint through its COM object model from PowerShell. Use the Office VBA documentation to determine object relationships, member names, parameter order, return types, units, and enum values; translate the VBA syntax into PowerShell.

The API facts below were checked against Microsoft's documentation on 2026-10-30. The PowerShell examples are original translations, not Microsoft-provided scripts. Validate them against the installed PowerPoint build and the actual presentation before relying on their results.

## Working instructions

1. Establish the input presentation, requested changes, output path, and whether the task concerns the saved file or the user's current unsaved session. Inspect the deck before choosing slides, shapes, layouts, or chart cells.
2. Use PowerShell COM for the document operations. Keep a reference to the intended `Presentation`; address its objects directly instead of relying on `ActivePresentation`, selection, clipboard state, mouse coordinates, or keyboard shortcuts.
3. For file editing, work on a separate copy unless an in-place edit was requested. A filesystem copy contains the saved state only. To capture a live presentation's unsaved state, use that presentation's `SaveCopyAs` and open the resulting copy.
4. Make the smallest change that fulfills the request. Preserve unrelated text formatting, themes, masters, layouts, grouping, notes, animations, links, and macros. Prefer editing an existing shape over deleting and rebuilding it.
5. Use inspected slide IDs and shape names where possible. Positions change when slides or shapes are inserted or deleted. Check match counts and stop on ambiguity rather than silently editing the first match.
6. Save explicitly, reopen the output to verify its contents, and inspect rendered slides when a change can affect layout. Report the output path, changes, checks performed, and any checks that could not be run.
7. Close only presentations opened by the script. Quit an application only when the session is known to be dedicated to this operation. Do not terminate all PowerPoint or Excel processes as cleanup.

Start API research at the [PowerPoint VBA reference](https://learn.microsoft.com/en-us/office/vba/api/overview/powerpoint) and [Application object](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.application). The documentation describes the automation object model; it does not require the calling code to be VBA.

## Runtime and connection

Use Windows with desktop PowerPoint installed and registered for COM. Run automation on the user's desktop in a single-threaded apartment (STA). Keep each COM session and its objects on one thread; do not pass COM objects into background jobs or parallel runspaces. This guide targets desktop editing, not a web service or Windows service. Microsoft documents limitations for [unattended, non-interactive Office automation](https://learn.microsoft.com/en-us/office/client-developer/integration/considerations-unattended-automation-office-microsoft-365-for-unattended-rpa).

Windows PowerShell 5.1 is a useful compatibility baseline. PowerShell 7 on Windows also exposes `New-Object -ComObject`; test the members needed by the task in the chosen runtime. An explicit STA invocation is:

```powershell
powershell.exe -NoProfile -STA -File .\Edit-Presentation.ps1
# Alternatively, with PowerShell 7 installed on Windows:
pwsh.exe -NoProfile -STA -File .\Edit-Presentation.ps1
```

Basic connection and inspection:

```powershell
$ErrorActionPreference = 'Stop'
if ([Threading.Thread]::CurrentThread.ApartmentState -ne 'STA') {
    throw 'Run this script in an STA PowerShell process.'
}
$app = New-Object -ComObject PowerPoint.Application
$presentations = $app.Presentations
Get-Member -InputObject $app
Get-Member -InputObject $presentations -Name Open
$app.Version
```

`New-Object` does not prove that the application process belongs exclusively to the script. PowerPoint can reuse an application server. For a live-session edit, enumerate `$app.Presentations` and match the intended `FullName`; don't assume the active deck is the target. In Windows PowerShell 5.1, `[Runtime.InteropServices.Marshal]::GetActiveObject('PowerPoint.Application')` is another attachment option. That .NET Framework API is not available as the same built-in method in PowerShell 7; do not copy it there without checking runtime support.

Sources: [New-Object](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/new-object), [PowerShell COM objects and Get-Member](https://learn.microsoft.com/en-us/powershell/scripting/samples/creating-.net-and-com-objects--new-object-), [Windows PowerShell STA option](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_powershell_exe?view=powershell-5.1), [PowerShell 7 STA option](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_pwsh), [Office application attachment and server reuse](https://learn.microsoft.com/en-us/previous-versions/office/troubleshoot/office-developer/use-visual-c-automate-run-program-instance).

## Translate VBA to PowerShell

| VBA concept | PowerShell COM form |
| --- | --- |
| `Set ppt = CreateObject("PowerPoint.Application")` | `$app = New-Object -ComObject PowerPoint.Application` |
| `Set s = pres.Slides(1)` | `$slide = $pres.Slides.Item(1)` |
| `shape.TextFrame.TextRange.Text = "Hello"` | `$shape.TextFrame.TextRange.Text = 'Hello'` |
| `pres.Save` | `$pres.Save()` |
| `pres.SaveAs FileName:=path, FileFormat:=ppSaveAsOpenXMLPresentation` | `$pres.SaveAs($path, 24)` |
| `With obj ... End With` | Assign `$obj`, then qualify each member access. |
| `Nothing` for an object parameter | Usually `$null`; follow the member's contract. |
| Omitted optional argument | Omit trailing arguments, or use `[Type]::Missing` for an intervening optional argument when the COM binder supports it. |
| `ByRef` output parameter | Use `[ref]$variable` only if the actual signature requires it. |
| VBA `Long` / `Single` | `[int]` / `[single]` when an explicit COM argument type is needed. |

PowerShell method calls use parentheses and positional arguments, not VBA's `Name:=value` syntax. A skipped optional value and a null object are different; don't substitute `$null` for every missing argument. Where binding is uncertain, supply documented arguments explicitly and inspect the installed signature with `Get-Member`.

Office collections and text positions generally start at **1**; PowerShell arrays start at **0**. Prefer `.Item($i)` and indexed loops. Avoid `1..$collection.Count` when the count can be zero: `1..0` produces two numbers. Suppress unwanted method results with `[void]`, or assign them to a variable. PowerShell functions emit every uncaptured expression, which can accidentally mix COM objects into a function's output.

A property documented as read-only can return a mutable object. For example, `TextRange.Font` cannot be replaced, but `TextRange.Font.Size` can be set. See [TextRange.Font](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.textrange.font).

### Constants

Late binding does not import VBA constant names. Define the values you use, or load compatible Office interop assemblies if already available. No interop assembly is required for the numeric forms here.

| Constant | Value | Use |
| --- | ---: | --- |
| `msoFalse`, `msoTrue` | `0`, `-1` | Office `MsoTriState` arguments/properties |
| `msoGroup`, `msoPlaceholder` | `6`, `14` | Shape types |
| `msoTextOrientationHorizontal` | `1` | Horizontal text box |
| `ppLayoutTitle`, `ppLayoutText` | `1`, `2` | Built-in slide layouts |
| `ppLayoutTitleOnly`, `ppLayoutBlank` | `11`, `12` | Built-in slide layouts |
| `ppPlaceholderBody` | `2` | Body placeholder, including notes body |
| `ppSaveAsOpenXMLPresentation` | `24` | `.pptx` |
| `ppSaveAsOpenXMLPresentationMacroEnabled` | `25` | `.pptm` |
| `ppSaveAsOpenXMLTemplate` | `26` | `.potx` |
| `ppSaveAsPDF` | `32` | PDF in the **SaveAs** enum |
| `ppFixedFormatTypePDF` | `2` | PDF in the **ExportAsFixedFormat** enum |
| `ppAlertsNone`, `ppAlertsAll` | `1`, `2` | PowerPoint `DisplayAlerts` |
| `msoAutomationSecurityForceDisable` | `3` | Disable macros while opening programmatically |

Use `-eq -1` to test a tri-state property for true. Mixed values are not equivalent to true. Some members, including `ChartData.IsLinked` and Excel `Workbook.Close`'s `SaveChanges`, use ordinary Boolean values instead. `DisplayAlerts` is an enum, not a Boolean.

Sources: [MsoTriState](https://learn.microsoft.com/en-us/office/vba/api/office.msotristate), [MsoShapeType](https://learn.microsoft.com/en-us/office/vba/api/office.msoshapetype), [MsoTextOrientation](https://learn.microsoft.com/en-us/office/vba/api/office.msotextorientation), [PpSlideLayout](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.ppslidelayout), [PpPlaceholderType](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.ppplaceholdertype), [PpSaveAsFileType](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.ppsaveasfiletype), [PpAlertLevel](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.ppalertlevel), [MsoAutomationSecurity](https://learn.microsoft.com/en-us/office/vba/api/office.msoautomationsecurity).

## Object model and inspection

```text
Application
  Presentations.Item(...) -> Presentation
    PageSetup -> SlideWidth, SlideHeight
    Designs.Item(...).SlideMaster -> CustomLayouts, Shapes
    Slides.Item(...) / Slides.FindBySlideID(...) -> Slide
      CustomLayout
      Shapes.Item(...) -> Shape
        TextFrame.TextRange -> Text, Font, ParagraphFormat
        GroupItems.Item(...) -> Shape
        Table.Cell(row, column).Shape -> cell text and formatting
        Chart.ChartData -> embedded or linked workbook
      NotesPage -> SlideRange -> Shapes
```

Inspect the presentation before editing. This example reports top-level shapes; groups and tables require the extra traversal described below.

```powershell
$slides = $pres.Slides
$inventory = for ($si = 1; $si -le $slides.Count; $si++) {
    $slide = $slides.Item($si)
    $shapes = $slide.Shapes
    for ($j = 1; $j -le $shapes.Count; $j++) {
        $shape = $shapes.Item($j)
        $text = ''
        if ($shape.HasTextFrame -eq -1) {
            if ($shape.TextFrame.HasText -eq -1) {
                $text = [string]$shape.TextFrame.TextRange.Text
            }
        }
        [pscustomobject]@{
            SlideIndex = $slide.SlideIndex
            SlideID = $slide.SlideID
            ShapeName = $shape.Name
            ShapeID = $shape.Id
            Type = [int]$shape.Type
            Text = $text
            Left = $shape.Left
            Top = $shape.Top
            Width = $shape.Width
            Height = $shape.Height
        }
    }
}
$inventory | Format-Table -AutoSize
```

Names such as `Title 1` are not universal. Use names discovered in this deck, or inspect placeholders by `PlaceholderFormat.Type`. Check `Shapes.HasTitle` before accessing `Shapes.Title`. A visible element may belong to a layout or slide master instead of the slide's own shapes. Editing a master can affect many slides; confirm that scope before changing it.

Sources: [TextRange](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.textrange), [HasTitle](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.shapes.hastitle), [PlaceholderFormat.Type](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.placeholderformat.type), [CustomLayouts](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.customlayouts).

## Complete example: edit a named text shape in a copy

Save this block as `Edit-Presentation.ps1`. Supply an inspected slide index and top-level shape name. It supports `.pptx` and `.pptm`, keeps the original extension, refuses to overwrite an existing output, changes one entire text range, saves, and reopens the result to check text and slide count. Use the narrower text recipe below when mixed formatting must be preserved.

```powershell
param(
    [Parameter(Mandatory)][string]$InputPath,
    [Parameter(Mandatory)][string]$OutputPath,
    [Parameter(Mandatory)][ValidateRange(1, 2147483647)][int]$SlideIndex,
    [Parameter(Mandatory)][string]$ShapeName,
    [Parameter(Mandatory)][AllowEmptyString()][string]$NewText
)

$ErrorActionPreference = 'Stop'
if ([Threading.Thread]::CurrentThread.ApartmentState -ne 'STA') {
    throw 'Run with powershell.exe -STA or pwsh.exe -STA on Windows.'
}

$source = (Resolve-Path -LiteralPath $InputPath).ProviderPath
$destination = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
$sourceExtension = [IO.Path]::GetExtension($source).ToLowerInvariant()
if ($sourceExtension -notin @('.pptx', '.pptm')) {
    throw 'This example expects a .pptx or .pptm input.'
}
if ([IO.Path]::GetExtension($destination).ToLowerInvariant() -ne $sourceExtension) {
    throw 'Keep the source extension for this example.'
}
if ($source -ieq $destination -or (Test-Path -LiteralPath $destination)) {
    throw 'Choose a new output path distinct from the input.'
}
$destinationFolder = Split-Path -Parent $destination
if (-not (Test-Path -LiteralPath $destinationFolder -PathType Container)) {
    throw 'The output directory must already exist.'
}
# File.Copy with overwrite=false also protects against an output created meanwhile.
[IO.File]::Copy($source, $destination, $false)

$app = $presentations = $pres = $check = $null
$slides = $slide = $shapes = $shape = $textRange = $null
$checkSlides = $checkSlide = $checkShapes = $checkShape = $null

function Get-PptUniqueShape {
    param($Shapes, [string]$Name)
    $found = $null
    for ($i = 1; $i -le $Shapes.Count; $i++) {
        $candidate = $Shapes.Item($i)
        if ([string]$candidate.Name -ceq $Name) {
            if ($null -ne $found) { throw "Duplicate shape name: $Name" }
            $found = $candidate
        }
    }
    if ($null -eq $found) { throw "Shape not found: $Name" }
    return $found
}

try {
    $app = New-Object -ComObject PowerPoint.Application
    $presentations = $app.Presentations
    $oldSecurity = $app.AutomationSecurity
    try {
        $app.AutomationSecurity = 3
        # Open(FileName, ReadOnly, Untitled, WithWindow)
        $pres = $presentations.Open($destination, 0, 0, 0)
    }
    finally {
        $app.AutomationSecurity = $oldSecurity
    }

    $slides = $pres.Slides
    $expectedSlideCount = $slides.Count
    if ($SlideIndex -gt $expectedSlideCount) { throw 'Slide index is out of range.' }
    $slide = $slides.Item($SlideIndex)
    $shapes = $slide.Shapes
    $shape = Get-PptUniqueShape -Shapes $shapes -Name $ShapeName
    if ($shape.HasTextFrame -ne -1) { throw 'The selected shape has no text frame.' }
    $textRange = $shape.TextFrame.TextRange
    $textRange.Text = $NewText
    # Capture PowerPoint's normalization of line endings for the reopen check.
    $expectedText = [string]$textRange.Text
    $pres.Save()

    $textRange = $shape = $shapes = $slide = $slides = $null
    $pres.Close()
    $pres = $null

    try {
        $app.AutomationSecurity = 3
        $check = $presentations.Open($destination, -1, 0, 0)
    }
    finally {
        $app.AutomationSecurity = $oldSecurity
    }
    $checkSlides = $check.Slides
    if ($checkSlides.Count -ne $expectedSlideCount) { throw 'Slide count verification failed.' }
    $checkSlide = $checkSlides.Item($SlideIndex)
    $checkShapes = $checkSlide.Shapes
    $checkShape = Get-PptUniqueShape -Shapes $checkShapes -Name $ShapeName
    if ([string]$checkShape.TextFrame.TextRange.Text -cne $expectedText) {
        throw 'Saved text verification failed.'
    }
    if ((Get-Item -LiteralPath $destination).Length -eq 0) { throw 'Output is empty.' }
    Write-Output "Verified output: $destination"
}
finally {
    $textRange = $shape = $shapes = $slide = $slides = $null
    $checkShape = $checkShapes = $checkSlide = $checkSlides = $null
    # These references belong only to presentations opened above.
    try {
        if ($null -ne $check) { $check.Close() }
    }
    finally {
        if ($null -ne $pres) { $pres.Close() }
        $check = $pres = $presentations = $app = $null
    }
}
```

On failure, this example closes its working copy and discards any unsaved partial edits. A file left at the output path is not evidence of success; it may still be the initial copy. For recovery-oriented interactive editing, retain the working presentation on failure instead and explicitly report its state. The script intentionally leaves the PowerPoint application running because process ownership is not assumed.

`Open(FileName, ReadOnly, Untitled, WithWindow)` returns a `Presentation`. `Untitled=-1` can also open an untitled copy; the example uses a distinct filesystem copy to make the output identity explicit. `WithWindow=0` hides the document window; it does not turn Office into a headless service. Do not rely on assigning `Application.Visible=0` to hide PowerPoint.

Macro security is changed only around opening and then restored. Editing through COM does not require enabling VBA macros. Do not bypass Protected View or other access restrictions to make an edit succeed. `DisplayAlerts` does not suppress security warnings.

Sources: [Presentations.Open](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.presentations.open), [AutomationSecurity](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.application.automationsecurity), [Presentation.Close](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.presentation.close).

## Editing recipes

These are independent snippets for use inside a controlled open/edit/save lifecycle. `$pres`, `$slide`, and `$shape` refer to objects already selected by inspection. Save after applying the requested recipes; the snippets do not save automatically.

### Text and formatting

For intentional replacement of an entire text box:

```powershell
if ($shape.HasTextFrame -ne -1) { throw 'Shape has no text frame.' }
$textRange = $shape.TextFrame.TextRange
$textRange.Text = "First paragraph`rSecond paragraph"
$textRange.Font.Size = 24
$textRange.Font.Bold = -1
$textRange.Paragraphs(1, 1).ParagraphFormat.Bullet.Visible = 0
```

Assigning the full `.Text` string can alter mixed character or paragraph formatting. For a local correction, work on the relevant `Characters`, `Paragraphs`, or `Runs` range and check the result. A small range edit reduces the affected area; it is not a guarantee that all formatting will be preserved. Use carriage return `` `r `` for paragraph breaks; verify imported line endings and paragraph counts. `TextFrame` and `TextFrame2` have different types and enums, so do not interchange their members blindly.

`TextRange.Replace(FindWhat, ReplaceWhat, After, MatchCase, WholeWords)` replaces the first match and returns its range, or `$null` if absent. This is a single replacement:

```powershell
$hit = $shape.TextFrame.TextRange.Replace('Old name', 'New name', 0, 0, 0)
if ($null -eq $hit) { throw 'Expected text was not found.' }
```

For deterministic replacement of all nonoverlapping literal, case-sensitive matches in a **whole text frame**, snapshot match positions and edit from right to left. This avoids matching text just inserted and avoids shifting positions not yet processed. The offsets below use ordinary .NET string indexing; validate behavior for unusual Unicode sequences in the target Office build.

```powershell
function Replace-PptLiteralText {
    param(
        [Parameter(Mandatory)]$TextRange,
        [Parameter(Mandatory)][ValidateNotNullOrEmpty()][string]$Find,
        [Parameter(Mandatory)][AllowEmptyString()][string]$Replacement
    )
    $original = [string]$TextRange.Text
    $positions = [Collections.Generic.List[int]]::new()
    $offset = 0
    while ($offset -lt $original.Length) {
        $index = $original.IndexOf($Find, $offset, [StringComparison]::Ordinal)
        if ($index -lt 0) { break }
        $positions.Add($index)
        $offset = $index + $Find.Length
    }
    for ($i = $positions.Count - 1; $i -ge 0; $i--) {
        $part = $TextRange.Characters($positions[$i] + 1, $Find.Length)
        $part.Text = $Replacement
        $part = $null
    }
    return $positions.Count
}
$changed = Replace-PptLiteralText -TextRange $shape.TextFrame.TextRange -Find 'FY25' -Replacement 'FY26'
```

Validate an intended `Characters(Start, Length)` range before using it: PowerPoint can clamp out-of-range arguments rather than throwing. Do not claim a presentation-wide replacement unless the required groups, cells, notes, and other text-bearing objects were included in traversal.

Sources: [TextRange.Replace](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.textrange.replace), [Characters](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.textrange.characters), [Paragraphs](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.textrange.paragraphs), [Runs](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.textrange.runs), [Text](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.textrange.text).

### Groups, table cells, and other text containers

When traversing a slide, inspect each shape. For `$shape.Type -eq 6`, recurse into `$shape.GroupItems.Item($i)` using indexes from 1 through `GroupItems.Count`. Edit children in place instead of ungrouping and regrouping. For `$shape.HasTable -eq -1`, walk `Table.Rows.Count` and `Table.Columns.Count` and access each `Table.Cell($row, $column).Shape.TextFrame.TextRange`. Handle merged cells according to the actual table; do not assume a rectangular traversal maps to distinct cells.

Text in charts, SmartArt, embedded OLE documents, layouts, and masters is not covered by an ordinary slide-shape text pass. Discover and handle these object types explicitly if the request includes them. [Shape.GroupItems](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.shape.groupitems), [Table.Cell](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.table.cell).

### Slides and layouts

```powershell
$slides = $pres.Slides
$sourceSlide = $slides.Item(1)
$recordedSlideId = $sourceSlide.SlideID

# Append a blank slide using a PpSlideLayout enum.
$blank = $slides.Add($slides.Count + 1, 12)

# Append using an existing, inspected CustomLayout object.
$layout = $sourceSlide.CustomLayout
$newSlide = $slides.AddSlide($slides.Count + 1, $layout)

# Duplicate returns a SlideRange; obtain its first Slide.
$duplicateRange = $sourceSlide.Duplicate()
$copy = $duplicateRange.Item(1)
$copy.MoveTo($slides.Count)

# Find the original after positions have changed.
$originalAgain = $slides.FindBySlideID($recordedSlideId)
```

Choose between a new slide and duplication according to the request. Duplicating a suitable existing slide often preserves the desired design and existing content. A `CustomLayout` index is specific to a master/design. If deleting several slides by position, delete in descending order, or resolve saved IDs individually; each deletion renumbers subsequent indexes.

Documentation caveat: the current `Slides.Add` page incorrectly shows `AddSlide` in parts of its syntax/example, although its Interop mapping identifies `Add(int, PpSlideLayout)`. Use `Add(index, layoutEnum)` versus `AddSlide(index, customLayoutObject)`. Similarly, use the `SlideRange` return type documented for `Duplicate`, despite contradictory introductory wording.

Sources: [Slides.Add](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.slides.add), [Slides.AddSlide](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.slides.addslide), [Duplicate](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.slide.duplicate), [MoveTo](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.slide.moveto), [FindBySlideID](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.slides.findbyslideid), [Delete](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.slide.delete).

### Shape geometry, text boxes, pictures, and color

Slide positions and sizes use **points**, with 72 points per inch. Read `PageSetup.SlideWidth` and `SlideHeight`; do not assume a widescreen deck. Moving or resizing an existing shape uses its `Left`, `Top`, `Width`, and `Height` properties.

```powershell
$box = $slide.Shapes.AddTextbox(1, 72, 72, 360, 72)
$box.Name = 'SummaryHeading'
$box.TextFrame.TextRange.Text = 'Quarterly summary'
$box.TextFrame.TextRange.Font.Size = 24

$imagePath = (Resolve-Path -LiteralPath 'C:\Work\chart.png').ProviderPath
# AddPicture(FileName, LinkToFile, SaveWithDocument, Left, Top, Width, Height)
# These dimensions are suitable only for an image with a 16:9 aspect ratio.
$picture = $slide.Shapes.AddPicture($imagePath, 0, -1, 72, 180, 360, 202.5)
$picture.LockAspectRatio = -1

function ConvertTo-OfficeRgb {
    param(
        [ValidateRange(0, 255)][int]$Red,
        [ValidateRange(0, 255)][int]$Green,
        [ValidateRange(0, 255)][int]$Blue
    )
    [int]($Red + 256 * $Green + 65536 * $Blue)
}
$box.TextFrame.TextRange.Font.Color.RGB = (ConvertTo-OfficeRgb 31 78 121)
```

For embedded pictures, `LinkToFile=0` requires `SaveWithDocument=-1`. Compute dimensions from the source image's aspect ratio before insertion; locking the ratio afterward does not undo an initial distortion. When replacing a picture, account for its crop, z-order, grouping, hyperlinks, and animation references. Adding a new picture and deleting the old one is not a full-fidelity replacement.

The RGB helper packs Office's color integer; a web `0xRRGGBB` value uses a different byte order. Explicit RGB formatting can override theme-dependent colors. Preserve theme formatting unless a fixed color is wanted. Also, `TextFrame.AutoSize` resizes the shape to fit its text; it does not mean shrinking text into a fixed box.

Sources: [SlideWidth](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.pagesetup.slidewidth), [AddTextbox](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.shapes.addtextbox), [AddPicture](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.shapes.addpicture), [ColorFormat.RGB](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.colorformat.rgb), [TextFrame.AutoSize](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.textframe.autosize).

### Tables

```powershell
# AddTable returns a Shape, whose Table property provides cell access.
$tableShape = $slide.Shapes.AddTable(3, 2, 40, 100, 600, 180)
$tableShape.Name = 'ResultsTable'
$table = $tableShape.Table
$cell = $table.Cell(1, 1)
$cell.Shape.TextFrame.TextRange.Text = 'Revenue'
$cell.Shape.TextFrame.TextRange.Font.Bold = -1
$table.Cell(1, 2).Shape.TextFrame.TextRange.Text = 'Amount'
$table.Cell(2, 1).Shape.TextFrame.TextRange.Text = 'Product A'
$table.Cell(2, 2).Shape.TextFrame.TextRange.Text = '$1,250'
```

For an existing table, first check `HasTable`, then edit the relevant cell shape. Row/column coordinates are 1-based. Formatting the entire table or recreating it may lose deliberate cell styling and merged structure. [Shapes.AddTable](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.shapes.addtable), [Table.Cell](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.table.cell).

### Speaker notes

Find the notes body by placeholder type instead of assuming it is placeholder number 2:

```powershell
$notesPage = $slide.NotesPage
$placeholders = $notesPage.Shapes.Placeholders
$notesBody = $null
for ($i = 1; $i -le $placeholders.Count; $i++) {
    $candidate = $placeholders.Item($i)
    if ($candidate.PlaceholderFormat.Type -eq 2 -and $candidate.HasTextFrame -eq -1) {
        if ($null -ne $notesBody) { throw 'Multiple notes body placeholders found.' }
        $notesBody = $candidate
    }
}
if ($null -eq $notesBody) { throw 'No notes body placeholder found.' }
$notesBody.TextFrame.TextRange.Text = 'Explain the assumptions before presenting the forecast.'
```

`NotesPage` returns a `SlideRange` representing that slide's notes page. Editing `NotesMaster` has presentation-wide scope. Notes pages support only a subset of `SlideRange` operations. [Slide.NotesPage](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.slide.notespage), [PpPlaceholderType](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.ppplaceholdertype).

### Embedded chart data

Inspect the chart's workbook, sheet names, series, and data range first. This focused recipe changes an already identified cell in an embedded workbook; it is not a general chart reconstruction procedure. Use it on a presentation copy opened by the script, with no pre-existing chart-data editing session. For a live-session edit, establish whether that workbook is already open and preserve the user's session instead of unconditionally closing it.

```powershell
if ($shape.HasChart -ne -1) { throw 'Shape has no chart.' }
$chart = $shape.Chart
$chartData = $chart.ChartData
if ($chartData.IsLinked) { throw 'Handle the external workbook explicitly for this linked chart.' }
$workbook = $worksheet = $range = $null
$chartEditSucceeded = $false
try {
    $chartData.Activate()
    $workbook = $chartData.Workbook
    $worksheet = $workbook.Worksheets.Item(1)
    $range = $worksheet.Range('B2')
    $range.Value2 = 42
    $chartEditSucceeded = $true
}
finally {
    $range = $worksheet = $null
    if ($null -ne $workbook) { $workbook.Close($chartEditSucceeded) }
    $workbook = $null
}
$chart.Refresh()
```

Call `ChartData.Activate()` before accessing `ChartData.Workbook`. This operation uses Excel and may display UI. Close only the workbook used for this edit; do not blindly quit its Excel application. `Refresh()` redraws the chart; it is not a promise to fetch external data or wait for all calculation. Save and reopen the presentation to verify both the data and chart appearance. Do not call `BreakLink()` merely to avoid handling a linked workbook.

Sources: [ChartData.Activate](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.chartdata.activate), [ChartData.Workbook](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.chartdata.workbook), [ChartData.IsLinked](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.chartdata.islinked), [Excel Workbook.Close](https://learn.microsoft.com/en-us/office/vba/api/excel.workbook.close), [Chart.Refresh](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.chart.refresh).

## Save and export

Use absolute paths and an explicit format when creating a differently named file. Preserve macro-enabled formats when macros must survive.

| Operation | PowerShell example | Meaning |
| --- | --- | --- |
| Save current presentation | `$pres.Save()` | Save changes to its current file. |
| Save under a new name | `$pres.SaveAs($pptxPath, 24)` | Save an editable `.pptx` and change the presentation's file identity. |
| Save macro-enabled | `$pres.SaveAs($pptmPath, 25)` | Save an editable `.pptm`. |
| Write a copy | `$pres.SaveCopyAs($copyPath, 24)` | Write a separate copy without changing the open presentation's identity. |
| Export a slide | `$slide.Export($pngPath, 'PNG', $pixelWidth, $pixelHeight)` | Render an image; does not save deck edits. |

For a PDF with slide output, print quality, and hidden slides excluded:

```powershell
# Path, FixedFormatType, Intent, FrameSlides, HandoutOrder,
# OutputType, PrintHiddenSlides, PrintRange, RangeType
$pres.ExportAsFixedFormat($pdfPath, 2, 2, 0, 1, 1, 0, $null, 1)
```

The VBA reference marks `PrintRange` as required, despite preceding optional arguments. Pass `$null` (VBA `Nothing`) when exporting all slides with `RangeType=1`. A selected slide range requires an actual `PrintRange` and the corresponding range type. Do not confuse `ppFixedFormatTypePDF=2` with `ppSaveAsPDF=32`.

For a PNG preview, preserve the actual slide aspect ratio. Export dimensions are **pixels**, whereas slide geometry is **points**:

```powershell
$pixelWidth = 1600
$pixelHeight = [int][Math]::Round(
    $pixelWidth * $pres.PageSetup.SlideHeight / $pres.PageSetup.SlideWidth
)
$slide.Export($pngPath, 'PNG', $pixelWidth, $pixelHeight)
```

Create output directories before saving/exporting, check for collisions, and ensure cloud-backed source files are fully downloaded. A PDF or preview image is additional output; retain the editable PowerPoint file when editing was requested.

Sources: [SaveAs](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.presentation.saveas), [SaveCopyAs](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.presentation.savecopyas), [PpSaveAsFileType](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.ppsaveasfiletype), [ExportAsFixedFormat](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.presentation.exportasfixedformat), [Slide.Export](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.slide.export).

## COM lifetime and application state

Always save before intentionally closing edited work. `Presentation.Close()` does not prompt to save. Assigning `Saved = -1` only marks the presentation as saved; it is not a disk write. Use `try/finally` to restore application settings and close owned documents, while keeping the failure behavior explicit.

Prefer short-lived scripts with a small number of retained COM references. Clear child references when finished. `Quit()` terminates the application, whereas clearing a variable or releasing its runtime-callable wrapper does not mean the application was closed. Only call `$app.Quit()` for a known dedicated session after its documents have been saved and closed.

If deterministic release is necessary, track all exclusively owned wrappers and release children before their parents with `Marshal.ReleaseComObject`. Never use a wrapper afterward. Avoid blanket `FinalReleaseComObject` calls or loops decrementing references to zero on potentially shared wrappers: aliases or other code may still use them. Chained property access and collection enumeration can create additional wrappers; assigning each child explicitly helps when implementing deterministic cleanup. Forced garbage collection is not a replacement for saving, closing, and ownership tracking.

Capture and restore `DisplayAlerts` if you change it; PowerPoint keeps the setting for the application session. Suppressing alerts can select default dialog responses and does not resolve security prompts or invalid operations.

Sources: [Presentation.Close](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.presentation.close), [Presentation.Saved](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.presentation.saved), [Application.Quit](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.application.quit), [Marshal.ReleaseComObject](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.interopservices.marshal.releasecomobject), [DisplayAlerts](https://learn.microsoft.com/en-us/office/vba/api/powerpoint.application.displayalerts).

## Verification and troubleshooting

After saving, reopen the output and verify the requested text/data, slide count and order, and relevant names/IDs. For visual changes, render the affected slides and inspect overflow, clipping, font substitution, overlap, picture cropping, chart labels, and unintended layout changes. Include nearby or representative slides when masters/layouts were changed. COM calls returning successfully do not establish visual correctness.

| Symptom | Check or response |
| --- | --- |
| Class not registered / cannot create COM object | Confirm desktop PowerPoint installation and COM registration for the chosen Windows runtime. Installing an interop assembly alone does not install PowerPoint. |
| Wrong argument type or member missing | Inspect the installed signature; check enum family, positional order, optional values, and actual returned object type. |
| `AddSlide` rejects a number | Use `Slides.Add(index, PpSlideLayout)` or pass a `CustomLayout` to `AddSlide`. |
| Text/shape lookup fails | Inspect actual names, placeholder types, group children, table cells, and master/layout scope. |
| Call rejected / application busy | Check for a modal dialog or active edit. Use bounded retries only for calls known to have been rejected before execution; inspect state before repeating mutations. |
| File open/save fails | Check full path, permissions, read-only/Protected View state, locks, download status, and output format. |
| Reopening the same file behaves differently by build | Reuse the existing presentation or open a distinct copy; do not assume two opens yield two independent objects. |
| Chart workbook access fails | Call `ChartData.Activate()` first and confirm Excel is available. |
| PowerPoint remains running | Check retained wrappers and whether the application is shared. A shared session should remain open; never use a blanket process kill. |
| Output exists but is wrong | Reopen it, check exact target values, and inspect rendered slides. File existence alone is insufficient. |

Microsoft documents a change starting with Microsoft 365 Apps Version 2508, Build 19127.20192, that prevents opening multiple copies of the same document through the object model. Use a distinct copied file where independent editing is needed. [Opening the same presentation twice](https://support.microsoft.com/en-au/support/known-issues/issues-using-vba-to-open-the-same-powerpoint-presentation-file-twice).

When reporting completion, distinguish documentation review, PowerShell syntax validation, actual COM execution, save/reopen checks, and visual inspection. Never describe an unexecuted example as runtime-tested.
