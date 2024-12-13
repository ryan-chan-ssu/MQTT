<?php
$servername = "127.0.0.1:3306";
$username = "u768038663_db_RyanChan";
$password = "@ndu1nWry4nN";
$dbname = "u768038663_RyanChan";

$conn = new mysqli($servername, $username, $password, $dbname);

// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
}

// Fetch the latest 100 records
$sql = "SELECT value, timestamp FROM potentiometer_data ORDER BY timestamp DESC LIMIT 100";
$result = $conn->query($sql);

$data = array();
while ($row = $result->fetch_assoc()) {
    $data[] = $row;
}

$conn->close();
header('Content-Type: application/json');
echo json_encode($data);
?>
