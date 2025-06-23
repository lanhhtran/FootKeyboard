#exit 0 
#--------------------------------------------------------------------------------
#    Nạp firmware lên thiết bị.
# @synopsis:  fetchcode.ps1 [COMx]
# @description: Tham số dòng lệnh duy nhất là cổng COm. Ví dụ fetchcode.ps1 [COM8
#                Nếu để trống, chương trình sẽ tự dò tìm các cổng COM đang có
# @dependences Phải cài gói esptool trước khi chạy lệnh này.
#--------------------------------------------------------------------------------


$BUILD_PATH=".\.pio\build\esp32-wroom-32"


$serialPorts = Get-PnpDevice -Class Ports | Select-Object Name, Status, InstanceId, DeviceID, Manufacturer, Service | Where-Object { $_.Status -eq "OK" -and  $_.DeviceID -match "USB"}
    # Display the status of each serial port
foreach ($port in $serialPorts) {
    # Xác định số hiệu cổng COM
    $serialPort = ([regex]'\((COM\d+)\)').Match($port.name).Groups[1].Value
    # Hiển thị ra màn hình 
    Write-Output "$serialPort"        
    Write-Output "    - Name: $($port.Name)"
    Write-Output "    - Manufacturer: $($port.Manufacturer)"
    Write-Output "    - Service: $($port.Service)"
    Write-Output "-----------------------------------"
}
if ($serialPorts.Count -gt 2) {
    Write-Output "Hãy chọn cổng COM mà bạn muốn nạp firmware vào thiết bị, và sử dụng tham số dòng lệnh để chỉ định chính xác cổng COM đó."
    exit 1
} elseif ($serialPorts.Count -eq 2) {
    if ($args[0] -ne $serialPort) {
        Write-Output "Cổng COM được phát hiện là $serialPort, nhưng bạn đã chỉ định cổng COM là $args[0]."
        Write-Output "Hãy chỉ định lại tham số dòng lệnh cổng COM cho chính xác, hoặc bỏ qua tham số đó để chương trình tự động dò tìm và chọn."
        exit 1
    }   
}

# Nạp firmware lên thiết bị
python -m esptool --chip esp32 `
    --port $serialPort --baud 115200 `
    write_flash -z `
    0x1000 $BUILD_PATH\bootloader.bin `
    0x8000 $BUILD_PATH\partitions.bin `
    0x10000 $BUILD_PATH\firmware.bin
