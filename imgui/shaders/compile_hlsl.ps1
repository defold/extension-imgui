param([string]$Dxc = 'dxc')

$ErrorActionPreference = 'Stop'
# Match Defold's DXC profiles and HLSL language semantics.
foreach ($stage in @(@('vert', 'vs_6_0'), @('frag', 'ps_6_0'))) {
    $base = Join-Path $PSScriptRoot "imgui_hlsl_60.$($stage[0])"
    & $Dxc -E main -T $stage[1] -HV 2018 -Ges -Fo "$base.dxil" "$base.hlsl"
    if ($LASTEXITCODE -ne 0) { throw "DXC failed for $base" }
}

# Both stages embed the same signature; package it separately for ShaderDesc.
& $Dxc -dumpbin -extractrootsignature -Fo "$PSScriptRoot/imgui_hlsl_60.rootsig" "$PSScriptRoot/imgui_hlsl_60.vert.dxil"
if ($LASTEXITCODE -ne 0) { throw 'DXC root signature extraction failed' }
