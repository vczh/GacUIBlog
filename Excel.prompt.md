# Edit Excel workbooks through PowerShell COM

Use this document as a reusable instruction prompt and API reference when editing Excel files on Windows. Operate desktop Microsoft Excel through its COM object model from PowerShell. Read the Office VBA reference for object relationships, parameter order, return types, units, and enum values; translate the calling syntax into PowerShell.

The API facts below were checked against Microsoft's documentation on 2026-10-30. The PowerShell examples are original translations, not Microsoft-provided scripts. Documentation review and syntax validation do not establish compatibility with every Excel build or workbook; verify the relevant operations on the installed application.

All 17 PowerShell blocks were syntax-checked in Windows PowerShell 5.1 and PowerShell 7.6.9. The complete copy/edit/save/reopen example was executed with Excel 16.0 in both shells. Additional PowerShell 7.6.9 checks covered literal text beginning with `=`, leading zeros, bulk arrays, A1 and R1C1 formulas, `FillDown`, and a `Formula2` dynamic-array spill with expected values. These checks used disposable workbooks; other recipes and the actual task's workbook still need appropriate validation.

## Working instructions

1. Establish the source workbook, target worksheets/ranges, requested changes, and output path. Distinguish a saved file from the user's unsaved Excel session.
2. Keep references to the intended `Workbook` and `Worksheet`. Qualify every `Range`, `Cells`, `Rows`, and `Columns` access; do not depend on `ActiveWorkbook`, `ActiveSheet`, selection, or the clipboard.
3. Work on a copy unless an in-place edit was requested. A filesystem copy contains saved bytes only. To capture the current state of a live workbook, use that workbook's `SaveCopyAs` and open the resulting copy.
4. Inspect formulas, number formats, named ranges, tables, protection, merged cells, hidden rows, and external connections before writing. Preserve formulas and workbook structure unless changing them is part of the task.
5. Separate literal data from formulas. Preserve identifiers and leading zeros as text. Use bulk rectangular writes when appropriate, without overwriting formula cells or unrelated formatting.
6. Save explicitly, reopen the result, and verify both formulas and values. Check rendering or PDF output when layout, charts, printing, or formatting changed.
7. Restore every application setting changed by the script. Close only owned workbooks and quit only an application instance dedicated to the operation. Never kill all Excel processes as cleanup.

Start at the [Excel VBA reference](https://learn.microsoft.com/en-us/office/vba/api/overview/excel) and [Application object](https://learn.microsoft.com/en-us/office/vba/api/excel.application). The same automation objects can be called from PowerShell; writing VBA macros into the workbook is unnecessary.

## Runtime and connection

Use Windows with desktop Excel installed and registered for COM. Windows PowerShell 5.1 is a useful compatibility baseline; PowerShell 7 on Windows also supports `New-Object -ComObject`. Use an STA process, keep COM references on that thread, and avoid passing them into background jobs or parallel runspaces. This guide covers desktop editing. Microsoft describes separate limitations for [unattended, non-interactive Office automation](https://learn.microsoft.com/en-us/office/client-developer/integration/considerations-unattended-automation-office-microsoft-365-for-unattended-rpa).

```powershell
powershell.exe -NoProfile -STA -File .\Edit-Workbook.ps1
# Or PowerShell 7 on Windows:
pwsh.exe -NoProfile -STA -File .\Edit-Workbook.ps1
```

Create a new automation instance, or deliberately attach to a known live session:

```powershell
$ErrorActionPreference = 'Stop'
if ([Threading.Thread]::CurrentThread.ApartmentState -ne 'STA') {
    throw 'Run this script in an STA PowerShell process.'
}
$app = New-Object -ComObject Excel.Application
$workbooks = $app.Workbooks
Get-Member -InputObject $workbooks -Name Open
$app.Version
```

In Windows PowerShell 5.1, `[Runtime.InteropServices.Marshal]::GetActiveObject('Excel.Application')` can attach to an application registered in the Running Object Table. That .NET Framework method is not available as the same built-in method in PowerShell 7. Attachment may not identify the particular Excel instance intended when several are running; enumerate its workbooks and match the full path. Do not use creation-style `Quit()` cleanup on an attached user session.

Sources: [New-Object](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.utility/new-object), [PowerShell COM objects](https://learn.microsoft.com/en-us/powershell/scripting/samples/creating-.net-and-com-objects--new-object-), [STA option](https://learn.microsoft.com/en-us/powershell/module/microsoft.powershell.core/about/about_powershell_exe?view=powershell-5.1), [Office application attachment](https://learn.microsoft.com/en-us/previous-versions/office/troubleshoot/office-developer/use-visual-c-automate-run-program-instance).

## Translate VBA to PowerShell

| VBA concept | PowerShell COM form |
| --- | --- |
| `Set xl = CreateObject("Excel.Application")` | `$app = New-Object -ComObject Excel.Application` |
| `Set ws = wb.Worksheets("Data")` | `$ws = $wb.Worksheets.Item('Data')` |
| `ws.Range("B2").Value2 = 42` | `$ws.Range('B2').Value2 = 42` |
| `ws.Cells(2, 3)` | `$ws.Cells.Item(2, 3)` |
| `wb.Save` | `$wb.Save()` |
| `wb.SaveAs Filename:=path, FileFormat:=xlOpenXMLWorkbook` | `$wb.SaveAs($path, 51)` |
| `Nothing` for an object | `$null`, when the member accepts a null object. |
| Omitted optional argument | Omit trailing arguments or use `[Type]::Missing` for an intervening optional value supported by the COM binder. |
| `ByRef` output parameter | Use `[ref]$variable` only when the actual signature requires it. |

Method calls take positional arguments in parentheses. VBA's `Name:=value` syntax is not PowerShell syntax. Missing, null, zero, and an empty string are distinct values. Inspect the installed signature with `Get-Member` if binding fails.

Excel collections, worksheet row/column numbers, and range-relative cell indexes are generally 1-based. PowerShell arrays normally start at 0. COM-returned arrays can have other lower bounds: inspect them instead of assuming. A multi-area `Range` is not one rectangular array. Suppress unused method results with `[void]`, and avoid returning unintended COM wrappers from helper functions.

Excel frequently uses ordinary Boolean properties: use `$true`/`$false` for `DisplayAlerts`, `EnableEvents`, `ScreenUpdating`, and `Close`'s `SaveChanges`. Office drawing objects may instead use `MsoTriState` (`msoTrue=-1`, `msoFalse=0`); follow the specific signature.

### Constants used below

| Constant | Value | Meaning |
| --- | ---: | --- |
| `xlOpenXMLWorkbook` | `51` | `.xlsx` |
| `xlOpenXMLWorkbookMacroEnabled` | `52` | `.xlsm` |
| `xlExcel12` | `50` | `.xlsb` |
| `xlExcel8` | `56` | Legacy `.xls` |
| `xlCSVUTF8` | `62` | UTF-8 CSV |
| `xlCalculationAutomatic`, `xlCalculationManual` | `-4105`, `-4135` | Calculation modes |
| `xlFormulas`, `xlValues` | `-4123`, `-4163` | Find's `LookIn` |
| `xlWhole`, `xlPart` | `1`, `2` | Find's `LookAt` |
| `xlByRows`, `xlByColumns` | `1`, `2` | Search order |
| `xlNext`, `xlPrevious` | `1`, `2` | Search direction |
| `xlYes`, `xlNo` | `1`, `2` | [Header presence](https://learn.microsoft.com/en-us/office/vba/api/excel.xlyesnoguess) |
| `xlSrcRange` | `1` | [Worksheet range as table source](https://learn.microsoft.com/en-us/office/vba/api/excel.xllistobjectsourcetype) |
| `xlAscending`, `xlDescending` | `1`, `2` | [Sort order](https://learn.microsoft.com/en-us/office/vba/api/excel.xlsortorder) |
| `xlColumnClustered`, `xlColumns` | `51`, `2` | Chart type and [series orientation](https://learn.microsoft.com/en-us/office/vba/api/excel.xlrowcol) |
| `xlTypePDF`, `xlQualityStandard` | `0`, `0` | PDF export type and [quality](https://learn.microsoft.com/en-us/office/vba/api/excel.xlfixedformatquality) |
| `msoAutomationSecurityForceDisable` | `3` | [Macro security when opening](https://learn.microsoft.com/en-us/office/vba/api/office.msoautomationsecurity) |

Do not reuse a number merely because another Office application uses it for a similar purpose. Excel's PDF export type is `0`, not PowerPoint's `2` or Word's `17`.

Sources: [XlFileFormat](https://learn.microsoft.com/en-us/office/vba/api/excel.xlfileformat), [XlCalculation](https://learn.microsoft.com/en-us/office/vba/api/excel.xlcalculation), [XlFindLookIn](https://learn.microsoft.com/en-us/office/vba/api/excel.xlfindlookin), [XlLookAt](https://learn.microsoft.com/en-us/office/vba/api/excel.xllookat), [XlSearchOrder](https://learn.microsoft.com/en-us/office/vba/api/excel.xlsearchorder), [XlSearchDirection](https://learn.microsoft.com/en-us/office/vba/api/excel.xlsearchdirection), [XlChartType](https://learn.microsoft.com/en-us/office/vba/api/excel.xlcharttype), [XlFixedFormatType](https://learn.microsoft.com/en-us/office/vba/api/excel.xlfixedformattype).

## Object model and inspection

```text
Application
  Workbooks.Item(...) -> Workbook
    Worksheets.Item(...) -> Worksheet
      Range(...), Cells.Item(row, column) -> Range
        Value2, Formula, Formula2, NumberFormat
        Font, Interior, Borders, Rows, Columns, Areas
      ListObjects.Item(...) -> ListObject
        HeaderRowRange, DataBodyRange, ListColumns, ListRows
      ChartObjects() -> ChartObject -> Chart
      PageSetup
    Names.Item(...) -> Name -> RefersToRange (when it refers to cells)
    Sheets -> worksheets and other sheet types, including chart sheets
    Connections, PivotCaches, Windows
```

```powershell
$worksheets = $wb.Worksheets
for ($i = 1; $i -le $worksheets.Count; $i++) {
    $ws = $worksheets.Item($i)
    $used = $ws.UsedRange
    [pscustomobject]@{
        Name = $ws.Name
        CodeName = $ws.CodeName
        UsedAddress = $used.Address()
        TableCount = $ws.ListObjects.Count
        Protected = $ws.ProtectContents
    }
}
```

`Worksheets` excludes chart sheets, while `Sheets` may include them. A name can refer to a formula or constant instead of a range, so `RefersToRange` is not universally available. Record actual sheet/table names; don't assume a workbook uses `Sheet1` or that table indexes remain fixed.

Treat `UsedRange` as an inspection hint, not a reliable count of data rows. Formatting and previous edits can extend it beyond meaningful data, and its first row/column need not be 1. Prefer a known table's data range or a task-specific bounded range. `CurrentRegion` follows blank-row/column boundaries and is unsuitable when those boundaries do not match the dataset. [Worksheet.UsedRange](https://learn.microsoft.com/en-us/office/vba/api/excel.worksheet.usedrange), [Range.CurrentRegion](https://learn.microsoft.com/en-us/office/vba/api/excel.range.currentregion).

## Complete example: edit a literal text cell in a copy

Save this block as `Edit-Workbook.ps1`. Supply a real worksheet name and one ordinary cell address. It preserves the source format (`.xlsx`, `.xlsm`, or `.xlsb`), changes that cell to literal text, saves, and verifies the persisted value. This intentionally changes the cell's number format to Text. It rejects formula, array, and merged cells rather than silently destroying their structure.

The example creates a new Excel automation instance dedicated to the operation and requires it to start without other workbooks. Do not replace its connection code with attachment to a user's application while retaining its `Quit()` cleanup.

```powershell
param(
    [Parameter(Mandatory)][string]$InputPath,
    [Parameter(Mandatory)][string]$OutputPath,
    [Parameter(Mandatory)][string]$SheetName,
    [Parameter(Mandatory)][string]$CellAddress,
    [Parameter(Mandatory)][AllowEmptyString()][string]$NewText
)
$ErrorActionPreference = 'Stop'
if ([Threading.Thread]::CurrentThread.ApartmentState -ne 'STA') {
    throw 'Run in an STA PowerShell process on Windows.'
}
$source = (Resolve-Path -LiteralPath $InputPath).ProviderPath
$destination = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputPath)
$extension = [IO.Path]::GetExtension($source).ToLowerInvariant()
if ($extension -notin @('.xlsx', '.xlsm', '.xlsb')) { throw 'Unsupported example input format.' }
if ([IO.Path]::GetExtension($destination).ToLowerInvariant() -ne $extension) {
    throw 'Keep the source extension for this example.'
}
if ($source -ieq $destination -or (Test-Path -LiteralPath $destination)) {
    throw 'Choose a new output path distinct from the input.'
}
if (-not (Test-Path -LiteralPath (Split-Path -Parent $destination) -PathType Container)) {
    throw 'The output directory must already exist.'
}
[IO.File]::Copy($source, $destination, $false)
$app = $workbooks = $wb = $check = $ws = $cell = $checkWs = $checkCell = $null
$old = $null
$oldCalculation = $null
$calculationChanged = $false
$dedicatedApplication = $false
try {
    $app = New-Object -ComObject Excel.Application
    $workbooks = $app.Workbooks
    if ($workbooks.Count -ne 0) { throw 'Expected an empty, dedicated automation instance.' }
    $dedicatedApplication = $true
    $old = @{
        Security = $app.AutomationSecurity
        Events = $app.EnableEvents
        ScreenUpdating = $app.ScreenUpdating
        Alerts = $app.DisplayAlerts
    }
    $app.EnableEvents = $false
    $app.ScreenUpdating = $false
    try {
        $app.AutomationSecurity = 3
        # Open(FileName, UpdateLinks, ReadOnly); 0 skips external formula-link updates.
        $wb = $workbooks.Open($destination, 0, $false)
    }
    finally { $app.AutomationSecurity = $old.Security }
    if ($wb.ReadOnly) { throw 'Working copy opened read-only.' }
    $oldCalculation = $app.Calculation
    $app.Calculation = -4135
    $calculationChanged = $true
    $sheetCount = $wb.Sheets.Count
    $ws = $wb.Worksheets.Item($SheetName)
    $cell = $ws.Range($CellAddress)
    if ($cell.Areas.Count -ne 1 -or $cell.Cells.CountLarge -ne 1) {
        throw 'Specify exactly one cell.'
    }
    if ($cell.MergeCells -or $cell.HasFormula -or $cell.HasArray) {
        throw 'This example edits only ordinary, unmerged, non-formula cells.'
    }
    $cell.NumberFormat = '@'
    $cell.Value2 = $NewText
    if ($cell.HasFormula) { throw 'The intended literal text became a formula.' }
    # Calculate dependencies in this dedicated instance; this does not refresh data sources.
    $app.Calculate()
    $app.Calculation = $oldCalculation
    $calculationChanged = $false
    $wb.Save()
    $cell = $ws = $null
    $wb.Close($false)
    $wb = $null
    try {
        $app.AutomationSecurity = 3
        $check = $workbooks.Open($destination, 0, $true)
    }
    finally { $app.AutomationSecurity = $old.Security }
    $checkWs = $check.Worksheets.Item($SheetName)
    $checkCell = $checkWs.Range($CellAddress)
    if ($check.Sheets.Count -ne $sheetCount -or $checkCell.HasFormula -or
        [string]$checkCell.Value2 -cne $NewText) {
        throw 'Saved workbook verification failed.'
    }
    Write-Output "Verified output: $destination"
}
finally {
    $cell = $ws = $checkCell = $checkWs = $null
    try {
        if ($calculationChanged -and $null -ne $wb) { $app.Calculation = $oldCalculation }
    }
    finally {
        try { if ($null -ne $check) { $check.Close($false) } }
        finally {
            try { if ($null -ne $wb) { $wb.Close($false) } }
            finally {
                if ($null -ne $old) {
                    $app.AutomationSecurity = $old.Security
                    $app.EnableEvents = $old.Events
                    $app.ScreenUpdating = $old.ScreenUpdating
                    $app.DisplayAlerts = $old.Alerts
                }
                if ($dedicatedApplication -and $workbooks.Count -eq 0) { $app.Quit() }
                $check = $wb = $workbooks = $app = $null
            }
        }
    }
}
```

On failure, the script discards unsaved changes to its own copy. A leftover output may contain only the initial source copy or a partially completed save; do not treat its existence as success. For interactive recovery, deliberately retain the working workbook and report its state instead.

`UpdateLinks=0` concerns external workbook/formula links; it is not a promise that every connection or refresh-on-open feature is disabled. Inspect those separately. `AutomationSecurity=3` disables VBA macros for programmatic opening, but Microsoft documents an exception for Excel 4.0 macros. Do not bypass Protected View or trust restrictions. [Workbooks.Open](https://learn.microsoft.com/en-us/office/vba/api/excel.workbooks.open), [AutomationSecurity](https://learn.microsoft.com/en-us/office/vba/api/excel.application.automationsecurity), [Workbook.Close](https://learn.microsoft.com/en-us/office/vba/api/excel.workbook.close).

## Values, formulas, dates, and locale

| Member | Use and limitation |
| --- | --- |
| `Value2` | Read/write cell values without VBA Currency/Date coercion. Formula cells return results; copying those results back destroys formulas. |
| `Formula` | Read/write A1 formulas with legacy implicit-intersection behavior. |
| `Formula2` | Dynamic-array-aware formula evaluation; results can spill. Check availability on the installed version. |
| `FormulaLocal` | Formula syntax in the user's language; avoid assuming English function names or separators. |
| `FormulaR1C1` | R1C1 formula notation, useful for relative formulas across ranges. |
| `Text` | Displayed text, affected by formatting and column width; unsuitable for lossless value extraction. |
| `NumberFormat` | Display format, independent of the underlying value. `NumberFormatLocal` uses local format syntax. |

```powershell
$ws.Range('B2').Value2 = 1250.5
$ws.Range('B2').NumberFormat = '#,##0.00'
$ws.Range('C2').Formula = '=B2*1.2'
# Single quotes prevent PowerShell from expanding $A and $B in Excel references.
$ws.Range('D2').Formula = '=$A$2+$B$2'
$ws.Range('E2').FormulaR1C1 = '=RC[-3]*2'
# Requires dynamic-array support and an unobstructed spill area.
$ws.Range('G2').Formula2 = '=SORT(A2:B20,2,-1)'
```

Write formulas only when intended. For a literal identifier such as `00123`, set `NumberFormat='@'` before writing a string. For untrusted strings that resemble formulas, use a text-format destination and verify `HasFormula` is false. A string that looks numeric may otherwise become a number; `Value2` is not a universal preservation guarantee.

Date values read through `Value2` are usually numeric serials. Inspect `Workbook.Date1904` before interpreting them; account for Excel's date-system conventions and the historical 1900 leap-day exception. Do not blindly apply `.FromOADate()` to every workbook/date range. Also distinguish a real date cell from an identifier whose number format happens to resemble a date. [Value2](https://learn.microsoft.com/en-us/office/vba/api/excel.range.value2), [Date1904](https://learn.microsoft.com/en-us/office/vba/api/excel.workbook.date1904).

Formula language, number-format language, text import delimiters, and workbook `SaveAs`'s `Local` argument are separate concerns. Avoid replacing punctuation in formula strings by guesswork. Validate formula text and calculated results after a locale-sensitive edit. [Formula](https://learn.microsoft.com/en-us/office/vba/api/excel.range.formula), [Formula2](https://learn.microsoft.com/en-us/office/vba/api/excel.range.formula2), [FormulaLocal](https://learn.microsoft.com/en-us/office/vba/api/excel.range.formulalocal), [NumberFormat](https://learn.microsoft.com/en-us/office/vba/api/excel.range.numberformat).

### Apply formulas to cells and columns

Choose a destination that is authorized to change. This small example puts quantity and price in B:C, then computes each row's total in D. Use it on a disposable or intentionally allocated range, not over existing workbook data.

```powershell
$inputs = [object[,]]::new(3, 2)
$inputs[0,0] = 2; $inputs[0,1] = 3
$inputs[1,0] = 4; $inputs[1,1] = 5
$inputs[2,0] = 6; $inputs[2,1] = 7
$ws.Range('B2:C4').Value2 = $inputs
$totals = $ws.Range('D2:D4')
$totals.NumberFormat = '0.00'
$ws.Range('D2').Formula = '=B2*C2'
[void]$totals.FillDown()
$totals.Calculate()
# Formulas: =B2*C2, =B3*C3, =B4*C4. Results: 6, 20, 42.
```

`FillDown()` copies the top row's contents **and formatting** through the destination, with relative references adjusted. Include the seed cell in the fill range. In the tested Excel 16.0 installation, direct assignment `$totals.Formula='=B2*C2'` also produced `=B2*C2`, `=B3*C3`, and `=B4*C4`; do not assume that a single A1 string stores identical literal references in every cell. Use absolute references when the referenced cells must remain fixed, and inspect individual cells' `Formula` properties. To express the relative offsets explicitly without copying the seed cell's formatting, use R1C1:

```powershell
# In each D cell, multiply the cells two columns and one column to its left.
$ws.Range('D2:D4').FormulaR1C1 = '=RC[-2]*RC[-1]'
# R2C2 is absolute B2; R[-1]C is the cell one row above the current cell.
```

In A1 notation, `B2` moves when copied, `$B$2` fixes row and column, `$B2` fixes the column, and `B$2` fixes the row. For example, set `$ws.Range('E2').Formula='=D2*(1+$H$1)'` and fill `E2:E4`: each row uses its own D value and the same H1 rate. Keep formulas in single-quoted PowerShell strings so `$H` is not expanded as a PowerShell variable. [FillDown](https://learn.microsoft.com/en-us/office/vba/api/excel.range.filldown), [FormulaR1C1](https://learn.microsoft.com/en-us/office/vba/api/excel.range.formular1c1).

For an existing table named `Orders` with `Qty`, `UnitPrice`, and `LineTotal` columns, apply a structured formula to the data cells of its calculated column:

```powershell
$orders = $ws.ListObjects.Item('Orders')
$lineTotals = $orders.ListColumns.Item('LineTotal').DataBodyRange
if ($null -eq $lineTotals) { throw 'The Orders table has no data rows.' }
$lineTotals.Formula = '=[@Qty]*[@UnitPrice]'
```

The `@` in these structured references identifies the current table row. Verify existing exceptions before replacing a whole calculated column. Avoid including the header or totals row in the formula assignment. [Structured table references](https://support.microsoft.com/en-us/excel/using-structured-references-with-excel-tables).

Dynamic arrays use `Formula2` on the anchor cell, with enough free cells for the spill. For example, `$ws.Range('J2').Formula2='=SEQUENCE(3,1,10,10)'` returns 10, 20, and 30 in J2:J4 on a supporting Excel version. Do not assign the formula separately to every spill cell. Clear only authorized obstructions, edit the anchor rather than a spill child, and keep spilled arrays outside Excel tables. Check legacy array formulas separately; rewriting only part of one is invalid. `Formula` and `Formula2` differ in implicit-intersection behavior, so a blind conversion can change results. [Formula versus Formula2](https://learn.microsoft.com/en-us/office/vba/excel/concepts/cells-and-ranges/range-formula-vs-formula2), [Formula2](https://learn.microsoft.com/en-us/office/vba/api/excel.range.formula2).

Verify formulas and results separately. For the B:C example above, the following detects a missing formula, an Excel error, or an incorrect result:

```powershell
$expected = @(6.0, 20.0, 42.0)
$ws.Calculate()
for ($i = 0; $i -lt $expected.Count; $i++) {
    $address = 'D' + ($i + 2)
    $resultCell = $ws.Range($address)
    if (-not $resultCell.HasFormula) { throw "Missing formula in $address" }
    if ($ws.Evaluate("ISERROR($address)")) { throw "Formula error in $address" }
    if ([Math]::Abs([double]$resultCell.Value2 - $expected[$i]) -gt 0.000001) {
        throw "Unexpected result in $address"
    }
}
```

Apply task-appropriate tolerances to numeric comparisons. `HasFormula` on a mixed multi-cell range can return null, so do not interpret a false-like value as proof that no formulas exist. Save and reopen to confirm the formulas remain formulas and their cached values are appropriate. [HasFormula](https://learn.microsoft.com/en-us/office/vba/api/excel.range.hasformula), [Worksheet.Evaluate](https://learn.microsoft.com/en-us/office/vba/api/excel.worksheet.evaluate).

## Bulk range reads and writes

For a contiguous multi-cell rectangle, read once, process locally, and write once. A single-cell result is a scalar; an empty cell can be `$null`. A multi-cell result is normally a two-dimensional COM array. Preserve its dimensions and lower bounds.

```powershell
$block = $ws.Range('A2:C100')
$values = $block.Value2
if ($values -is [Array] -and $values.Rank -eq 2) {
    for ($r = $values.GetLowerBound(0); $r -le $values.GetUpperBound(0); $r++) {
        for ($c = $values.GetLowerBound(1); $c -le $values.GetUpperBound(1); $c++) {
            $value = $values.GetValue($r, $c)
            # Inspect or transform this local value according to the task.
        }
    }
} else {
    $singleValue = $values
}
```

Create a genuine rectangular array, not an array of row arrays, for an intentional values-only write:

```powershell
$data = [object[,]]::new(3, 2)
$data[0,0] = 'Item'; $data[0,1] = 'Amount'
$data[1,0] = 'Coffee'; $data[1,1] = 12.5
$data[2,0] = 'Tea'; $data[2,1] = 8.0
$destinationRange = $ws.Range('A1:B3')
$destinationRange.Value2 = $data
```

Match array dimensions to destination dimensions. Do not bulk-write `Value2` across formulas, merged ranges, or discontiguous areas. Existing cell formats are separate from values, but Excel may interpret incoming values; set text-format columns first when required. `Resize` and `Offset` return ranges; always retain the correct worksheet ownership. Avoid per-cell COM loops for large data and avoid reading entire worksheets unnecessarily. [Range.Value](https://learn.microsoft.com/en-us/office/vba/api/excel.range.value).

## Find and precise targeting

`Range.Find` remembers several options, including options set through the Find dialog. Supply them explicitly. `After` must be one cell inside the search range, and the search starts after it. The result is a `Range` or `$null`; searches wrap around.

```powershell
$searchRange = $ws.Range('A2:A1000')
$after = $searchRange.Cells.Item($searchRange.Cells.Count)
# What, After, LookIn, LookAt, SearchOrder, SearchDirection,
# MatchCase, MatchByte, SearchFormat
$hit = $searchRange.Find('Order-1042', $after, -4163, 1, 1, 1, $false, $false, $false)
if ($null -eq $hit) { throw 'Expected identifier not found.' }
$firstAddress = $hit.Address()
$addresses = [Collections.Generic.List[string]]::new()
do {
    $addresses.Add($hit.Address())
    $hit = $searchRange.Find('Order-1042', $hit, -4163, 1, 1, 1, $false, $false, $false)
} while ($null -ne $hit -and $hit.Address() -ne $firstAddress)
if ($addresses.Count -ne 1) { throw 'Identifier is not unique.' }
$rowNumber = $ws.Range($addresses[0]).Row
```

Collect matches before mutating their contents. Stop when the first address repeats; otherwise a search can loop forever. `xlValues` and `xlFormulas` search different representations. Excel search wildcards (`*`, `?`, and `~` escapes) are not regular expressions. Literal matching of arbitrary user strings requires escaping wildcard characters or comparing locally read values. [Range.Find](https://learn.microsoft.com/en-us/office/vba/api/excel.range.find).

## Formatting, rows, and columns

```powershell
$header = $ws.Range('A1:D1')
$header.Font.Bold = $true
$header.Interior.Color = [int](31 + 256*78 + 65536*121)
$header.Font.Color = [int](255 + 256*255 + 65536*255)
$ws.Range('D2:D100').NumberFormat = '0.0%'
$ws.Range('A2:A100').WrapText = $true
[void]$ws.Range('A:D').Columns.AutoFit()
$ws.Range('A:A').ColumnWidth = 24
$ws.Range('2:100').Rows.AutoFit()
```

`ColumnWidth` is based on the workbook's normal-font character width, not points. `RowHeight` and drawing-object geometry use points. AutoFit can produce excessively wide columns; inspect and cap widths as appropriate. Merged cells need separate layout attention. Preserve custom number formats and conditional formatting rather than replacing them with direct formatting accidentally.

Use `ClearContents()` to remove values/formulas while retaining formatting; `Clear()` also removes formatting. `Delete()` shifts cells or removes entire rows/columns, so specify the intended scope. Inserting/deleting rows can affect names, formulas, tables, chart ranges, and content below the target. Delete multiple row positions from bottom to top, or resolve stable identifiers after each mutation. [ClearContents](https://learn.microsoft.com/en-us/office/vba/api/excel.range.clearcontents), [Range.Delete](https://learn.microsoft.com/en-us/office/vba/api/excel.range.delete), [ColumnWidth](https://learn.microsoft.com/en-us/office/vba/api/excel.range.columnwidth).

## Tables, sorting, and filters

Use `ListObject` for an Excel table, not just a visually formatted rectangle. The source below already contains unique text headers. Omit `LinkSource` for `xlSrcRange`; passing `$false` there is still supplying an invalid argument.

```powershell
$missing = [Type]::Missing
$sourceRange = $ws.Range('A1:B3')
# SourceType, Source, LinkSource, XlListObjectHasHeaders
$table = $ws.ListObjects.Add(1, $sourceRange, $missing, 1)
$table.Name = 'SalesData'
$table.TableStyle = 'TableStyleMedium2'
$body = $table.DataBodyRange
if ($null -ne $body) { $body.Columns.Item(2).NumberFormat = '#,##0.00' }
$newRow = $table.ListRows.Add($missing, $false)
$newRow.Range.Cells.Item(1, 1).Value2 = 'Cocoa'
$newRow.Range.Cells.Item(1, 2).Value2 = 9.5
```

An empty table may have no `DataBodyRange`; guard it. `ListRows.Add(...,$false)` uses the empty row below when possible, but still shifts occupied cells below the table. Inspect that space first. Adding a table can convert header values to text and disambiguate duplicate header names. [ListObjects.Add](https://learn.microsoft.com/en-us/office/vba/api/excel.listobjects.add), [DataBodyRange](https://learn.microsoft.com/en-us/office/vba/api/excel.listobject.databodyrange), [ListRows.Add](https://learn.microsoft.com/en-us/office/vba/api/excel.listrows.add).

Sort complete records together. For a plain rectangular dataset with one header row, this sorts by column B ascending, keeping the other columns attached:

```powershell
$missing = [Type]::Missing
$records = $ws.Range('A1:D100')
$key = $ws.Range('B2')
# Key1, Order1, Key2, Type, Order2, Key3, Order3, Header
[void]$records.Sort($key, 1, $missing, $missing, $missing, $missing, $missing, 1)
# Field is relative to the supplied range: 2 means its second column.
[void]$records.AutoFilter(2, 'Approved')
```

Use the table's `Sort` object for a table-specific sort and inspect current filters before replacing them. AutoFilter hides nonmatching rows; it does not remove their cells. Writing to the original range still affects hidden rows. If the request concerns only visible cells, explicitly select and handle the resulting areas. Calling `AutoFilter()` without arguments toggles the arrows rather than clearing all criteria predictably. [Range.Sort](https://learn.microsoft.com/en-us/office/vba/api/excel.range.sort), [Range.AutoFilter](https://learn.microsoft.com/en-us/office/vba/api/excel.range.autofilter).

## Charts and other workbook objects

```powershell
$chartObjects = $ws.ChartObjects()
$chartObject = $chartObjects.Add(360, 30, 420, 240)
$chartObject.Name = 'SalesChart'
$chart = $chartObject.Chart
$chart.ChartType = 51
$chart.SetSourceData($ws.Range('A1:B3'), 2)
$chart.HasTitle = $true
$chart.ChartTitle.Text = 'Sales by product'
```

`ChartObjects.Add` returns the embedded chart's container; `.Chart` provides its chart API. Its coordinates and size are in points. For an existing chart, edit its source/series and formatting instead of recreating it unnecessarily. Verify that categories, series names, units, hidden-data behavior, and formulas match the intended range. [ChartObjects.Add](https://learn.microsoft.com/en-us/office/vba/api/excel.chartobjects.add), [Chart.SetSourceData](https://learn.microsoft.com/en-us/office/vba/api/excel.chart.setsourcedata).

Named ranges, PivotTables, conditional formats, validation rules, notes/threaded comments, and shapes each have their own APIs. Preserve them during ordinary cell edits. A PivotTable result area is not a normal data-entry range; edit the underlying source and use the appropriate refresh operation. Do not replace a native chart or table with an image when editability is required.

## Calculation and external data

`Application.Calculation` is application-wide. Capture its previous value, change it only when beneficial, and restore it in `finally`. Set it after a workbook is open; some Excel contexts reject a mode change with no workbook. Restoring automatic calculation can trigger work, so do it deliberately before final saving when the output must retain that mode.

Choose the necessary calculation scope: `$range.Calculate()`, `$ws.Calculate()`, or `$app.Calculate()` (all open workbooks). Limited scope may not resolve dependencies outside it. `CalculateFullRebuild()` is a costly application-wide dependency rebuild, not a default step for every cell change. Formula caches may be stale until calculation completes. [Calculation](https://learn.microsoft.com/en-us/office/vba/api/excel.application.calculation), [Calculate](https://learn.microsoft.com/en-us/office/vba/api/excel.application.calculate).

Recalculation and data refresh are separate. `Workbook.RefreshAll()` starts refresh of external ranges and PivotTables; background work can outlive the call. Microsoft's current reference also documents Power Query table failures that do not surface as a `RefreshAll` run-time error, leaving old data in place. Where appropriate, refresh that table's `QueryTable` synchronously and inspect errors/results. `CalculateUntilAsyncQueriesDone()` concerns pending OLEDB/OLAP queries; it is not a universal completion check for every connection type. Use connection-specific completion checks and bounded waits before validating and saving. [RefreshAll](https://learn.microsoft.com/en-us/office/vba/api/excel.workbook.refreshall), [CalculateUntilAsyncQueriesDone](https://learn.microsoft.com/en-us/office/vba/api/excel.application.calculateuntilasyncqueriesdone).

## Save, print layout, and PDF export

| Operation | PowerShell form | Purpose |
| --- | --- | --- |
| Save current file | `$wb.Save()` | Persist edits to its current file. |
| Save as `.xlsx` | `$wb.SaveAs($xlsxPath, 51)` | Change workbook identity and format. |
| Save as `.xlsm` | `$wb.SaveAs($xlsmPath, 52)` | Preserve VBA-capable format. |
| Save a same-format copy | `$wb.SaveCopyAs($copyPath)` | Write a copy without changing the live workbook. |
| Close after saving | `$wb.Close($false)` | Close without performing another save. |

Use absolute paths and an extension matching the selected file format. Preserve `.xlsm`/`.xlsb` when the workbook requires macros. A CSV export represents flat sheet data; it does not preserve workbook formulas, styles, multiple sheets, or other workbook objects. `SaveCopyAs` has no `FileFormat` argument. [Workbook.SaveAs](https://learn.microsoft.com/en-us/office/vba/api/excel.workbook.saveas), [Workbook.SaveCopyAs](https://learn.microsoft.com/en-us/office/vba/api/excel.workbook.savecopyas), [XlFileFormat](https://learn.microsoft.com/en-us/office/vba/api/excel.xlfileformat).

For one worksheet's PDF, configure the intended print area and inspect pagination:

```powershell
$pageSetup = $ws.PageSetup
$pageSetup.PrintArea = '$A$1:$F$40'
$pageSetup.Orientation = 2  # xlLandscape
$pageSetup.Zoom = $false
$pageSetup.FitToPagesWide = 1
$pageSetup.FitToPagesTall = $false
$missing = [Type]::Missing
# Type, FileName, Quality, IncludeDocProperties, IgnorePrintAreas, From, To, OpenAfterPublish
$ws.ExportAsFixedFormat(0, $pdfPath, 0, $true, $false, $missing, $missing, $false)
```

`Zoom` must be false for fit-to-pages settings to control scaling. One page wide does not force one page tall. Preserve print titles, margins, paper size, headers/footers, and existing print areas unless changing them is requested. Use `Workbook.ExportAsFixedFormat` for workbook-level output with an explicitly understood sheet scope. PDF export is additional output and does not replace saving the editable workbook. [FitToPagesWide](https://learn.microsoft.com/en-us/office/vba/api/excel.pagesetup.fittopageswide), [XlPageOrientation](https://learn.microsoft.com/en-us/office/vba/api/excel.xlpageorientation), [Worksheet.ExportAsFixedFormat](https://learn.microsoft.com/en-us/office/vba/api/excel.worksheet.exportasfixedformat).

## Application state and COM lifetime

Capture and restore `ScreenUpdating`, `EnableEvents`, `DisplayAlerts`, `Calculation`, and any other settings you change. These affect the application, including other open workbooks in an attached session. `EnableEvents=$false` suppresses event handlers, not every possible source of macro execution. Set and restore `AutomationSecurity` immediately around programmatic opening.

Do not use `DisplayAlerts=$false` as a universal error handler. Excel may choose an automatic response, including overwriting an existing file during `SaveAs`. Cross-process automation must restore the setting explicitly. When possible, keep alerts unchanged and prevent collisions through path checks. [DisplayAlerts](https://learn.microsoft.com/en-us/office/vba/api/excel.application.displayalerts), [EnableEvents](https://learn.microsoft.com/en-us/office/vba/api/excel.application.enableevents).

Save before closing. `Workbook.Close($false)` discards unsaved work; `$true` asks Excel to save, and an unnamed workbook may still need a filename. Setting `Workbook.Saved=$true` only clears the dirty flag; it does not persist edits. Application `Quit()` affects the whole instance. Track which workbooks and application the script owns, including workbooks created indirectly by chart or external-data operations.

Prefer short-lived scripts and few retained COM wrappers. Clear child references when finished. If deterministic release is necessary, track exclusively owned wrappers and release children before parents with `Marshal.ReleaseComObject`. Do not use a released wrapper again, and do not force-release shared aliases or repeatedly decrement arbitrary wrappers to zero. Chained property access and COM enumeration can create additional wrappers. Garbage collection does not substitute for explicit saving and closing. [Marshal.ReleaseComObject](https://learn.microsoft.com/en-us/dotnet/api/system.runtime.interopservices.marshal.releasecomobject), [Application.Quit](https://learn.microsoft.com/en-us/office/vba/api/excel.application.quit).

## Verification and troubleshooting

Reopen the saved workbook and verify the requested target values, formulas, sheet/table names, and relevant row counts. Check formula errors and expected totals after the required calculation/refresh. Preserve workbook date systems, number formats, leading zeros, links, and macros. For visual changes, inspect the affected sheets/charts and any exported pages for truncation, `####`, unreadable scaling, and unintended page breaks.

| Symptom | Check or response |
| --- | --- |
| COM class unavailable | Confirm desktop Excel and COM registration for the chosen Windows runtime. Interop assemblies alone do not install Excel. |
| Cell edit affects the wrong workbook | Qualify worksheet/range references; eliminate `ActiveSheet` and unqualified `Cells`. |
| Identifier loses zeros or becomes a formula | Format as text before writing a string, then verify value and `HasFormula`. |
| Scalar/array indexing fails | Handle single-cell/null values separately; inspect 2-D array lower bounds. |
| Large empty area appears in `UsedRange` | Inspect real dataset boundaries instead of treating formatting as data. |
| Find results change between runs | Set every relevant Find option and detect wraparound. |
| Table creation rejects an argument | Omit `LinkSource` for `xlSrcRange`; check source/header semantics. |
| Spill/array edit fails | Edit the owning formula or intended input cells; do not write into a spill result. |
| Values remain stale | Distinguish formula calculation from connection refresh and confirm completion/results. |
| Save or export fails | Check read-only/protection state, full paths, output collisions, permissions, and matching format. |
| Excel remains running | Inspect retained wrappers and owned documents; preserve shared sessions instead of killing processes. |

When reporting completion, distinguish documentation review, syntax checks, actual Excel COM execution, save/reopen validation, and visual inspection. Do not describe unexecuted recipes as runtime-tested.
