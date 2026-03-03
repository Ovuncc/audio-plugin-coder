$content = Get-Content -Raw 'plugins/Experimental_osmium_v3/Source/PluginProcessor.cpp'
$idsText = Get-Content -Raw 'plugins/Experimental_osmium_v3/Source/ParameterIDs.hpp'

$rows = @()

$idMap = @{}
[regex]::Matches($idsText, 'static constexpr const char\*\s+(?<k>\w+)\s*=\s*"(?<v>[a-z0-9_]+)"') | ForEach-Object {
  $idMap[$_.Groups['k'].Value] = $_.Groups['v'].Value
}

$floatPattern = [regex]'AudioParameterFloat>\(\s*juce::ParameterID\(ParameterIDs::(?<id>\w+),\s*1\),\s*"(?<name>[^"]+)",\s*juce::NormalisableRange<float>\((?<range>[^\)]*)\),\s*(?<def>[-0-9\.]+)f?\s*\)\s*\)'
$choicePattern = [regex]'AudioParameterChoice>\(\s*juce::ParameterID\(ParameterIDs::(?<id>\w+),\s*1\),\s*"(?<name>[^"]+)",\s*juce::StringArray\s*\{\s*(?<choices>[^\}]*)\},\s*(?<def>\d+)\s*\)\s*\)'
$boolPattern = [regex]'AudioParameterBool>\(\s*juce::ParameterID\(ParameterIDs::(?<id>\w+),\s*1\),\s*"(?<name>[^"]+)",\s*(?<def>true|false)\s*\)\s*\)'

function Get-ModeBlock([string]$id){
  switch -Regex ($id) {
    '^intensity$|^outputGain$|^bypass$|^expManualMode$|^expMuteLowBand$|^expMuteHighBand$|^expCrossoverHz$|^expOversamplingMode$' { return 'Global' }
    '^expLowLimiter' { return 'Low Limiter' }
    '^expLow(Fast|Slow|AttackGain|CompAttackRatio|CompGain|Transient)' { return 'Low Detector' }
    '^expLow' { return 'Low Core' }
    '^expHigh(Fast|Slow|AttackGain|CompAttackRatio|CompGain|Transient)' { return 'High Detector' }
    '^expHigh' { return 'High Core' }
    '^expTight(Fast|Slow|Program|Transient|Threshold|SustainCurve)' { return 'Tight Detector' }
    '^expTight' { return 'Tight Core' }
    '^expOutput' { return 'Output Shaping' }
    '^expLimiter' { return 'Output Limiter' }
    default { return 'Other' }
  }
}

function Convert-Id([string]$id){
  if($idMap.ContainsKey($id)){ return $idMap[$id] }
  return $id
}

$floatPattern.Matches($content) | ForEach-Object {
  $id = $_.Groups['id'].Value
  $rangeParts = $_.Groups['range'].Value -split ','
  $min = ($rangeParts[0] -replace 'f','').Trim()
  $max = ($rangeParts[1] -replace 'f','').Trim()
  $def = ($_.Groups['def'].Value -replace 'f','').Trim()
  $rows += [pscustomobject]@{
    mode_block = Get-ModeBlock $id
    param_id = Convert-Id $id
    type = 'float'
    min = $min
    max = $max
    default = $def
    choices = ''
  }
}

$choicePattern.Matches($content) | ForEach-Object {
  $id = $_.Groups['id'].Value
  $choices = ($_.Groups['choices'].Value -split ',') | ForEach-Object { ($_ -replace '"','').Trim() }
  $defIdx = [int]$_.Groups['def'].Value
  $defVal = if($defIdx -ge 0 -and $defIdx -lt $choices.Count){ $choices[$defIdx] } else { "$defIdx" }
  $rows += [pscustomobject]@{
    mode_block = Get-ModeBlock $id
    param_id = Convert-Id $id
    type = 'choice'
    min = '0'
    max = ([string]($choices.Count - 1))
    default = $defVal
    choices = ($choices -join '|')
  }
}

$boolPattern.Matches($content) | ForEach-Object {
  $id = $_.Groups['id'].Value
  $def = $_.Groups['def'].Value
  $rows += [pscustomobject]@{
    mode_block = Get-ModeBlock $id
    param_id = Convert-Id $id
    type = 'bool'
    min = 'false'
    max = 'true'
    default = $def
    choices = 'false|true'
  }
}

$ordered = $rows | Sort-Object @{Expression='mode_block';Descending=$false}, @{Expression='param_id';Descending=$false}
$csv = $ordered | ConvertTo-Csv -NoTypeInformation
$csv -join "`n"
