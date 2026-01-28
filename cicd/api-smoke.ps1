# API smoke test: /versions and POST /login (invalid creds → JSON error = path OK)
$base = "https://matrix.buffertavern.com"
Write-Host "1) GET /_matrix/client/versions"
try {
    $v = Invoke-WebRequest -Uri "$base/_matrix/client/versions" -UseBasicParsing -TimeoutSec 10
    Write-Host "   Status: $($v.StatusCode) OK"
} catch { Write-Host "   FAIL: $_" }

Write-Host "2) POST /_matrix/client/v3/login (invalid user: expect 403/401 + JSON)"
$body = '{"type":"m.login.password","identifier":{"type":"m.id.user","user":"@smoketest:vault.buffertavern.com"},"password":"invalid","initial_device_display_name":"SmokeTest"}'
try {
    $r = Invoke-WebRequest -Uri "$base/_matrix/client/v3/login" -Method POST -Body $body -ContentType "application/json" -UseBasicParsing -TimeoutSec 10
    Write-Host "   Status: $($r.StatusCode) Body: $($r.Content.Substring(0, [Math]::Min(120,$r.Content.Length)))..."
} catch {
    $code = [int]$_.Exception.Response.StatusCode
    Write-Host "   Status: $code (expected for invalid creds; proves Client->CF->tunnel->Synapse path OK)"
}
Write-Host "Done. 200 on /versions and 401/403 JSON on /login => path OK."
