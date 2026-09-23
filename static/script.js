// ============================================================
// VIGIDRIVE DASHBOARD JAVASCRIPT
// ============================================================


// Canvas used for the live impact graph
const chartCanvas =
    document.getElementById("impactChart");

const ctx =
    chartCanvas.getContext("2d");


// ============================================================
// SIMPLE TEXT UPDATE FUNCTION
// ============================================================

function setText(id, value) {

    const element =
        document.getElementById(id);

    if (element) {

        element.textContent = value;
    }
}


// ============================================================
// FORMAT NUMBERS
// ============================================================

function formatNumber(
    value,
    digits = 2
) {

    if (
        value === null ||
        value === undefined ||
        Number.isNaN(Number(value))
    ) {

        return "—";
    }

    return Number(value).toFixed(digits);
}


// ============================================================
// ACCIDENT STATUS COLOR
// ============================================================

function setStatusClass(status) {

    const element =
        document.getElementById(
            "accidentStatus"
        );


    element.className = "";


    if (status === "Normal") {

        element.classList.add(
            "status-normal"
        );

    }

    else if (status === "Warning") {

        element.classList.add(
            "status-warning"
        );

    }

    else {

        element.classList.add(
            "status-danger"
        );
    }
}


// ============================================================
// DRAW LIVE IMPACT GRAPH
// ============================================================

function drawChart(points) {

    const rect =
        chartCanvas.getBoundingClientRect();


    const dpr =
        window.devicePixelRatio || 1;


    chartCanvas.width =
        rect.width * dpr;

    chartCanvas.height =
        rect.height * dpr;


    ctx.setTransform(
        dpr,
        0,
        0,
        dpr,
        0,
        0
    );


    const width =
        rect.width;

    const height =
        rect.height;


    ctx.clearRect(
        0,
        0,
        width,
        height
    );


    // No data yet

    if (
        !points ||
        points.length < 2
    ) {

        ctx.fillStyle =
            "#8ea2bb";

        ctx.font =
            "13px Segoe UI";

        ctx.fillText(
            "Waiting for acceleration data...",
            20,
            30
        );

        return;
    }


    // Extract G values

    const values =
        points.map(
            point =>
                Number(point.g) || 0
        );


    const maxValue =
        Math.max(
            2.0,
            ...values
        ) * 1.15;


    const minValue = 0;


    // ========================================================
    // GRID
    // ========================================================

    ctx.strokeStyle =
        "rgba(255,255,255,0.07)";

    ctx.lineWidth = 1;


    for (
        let i = 0;
        i <= 4;
        i++
    ) {

        const y =
            15 +
            (
                height - 35
            ) *
            i /
            4;


        ctx.beginPath();

        ctx.moveTo(
            0,
            y
        );

        ctx.lineTo(
            width,
            y
        );

        ctx.stroke();
    }


    // ========================================================
    // GRAPH LINE
    // ========================================================

    ctx.strokeStyle =
        "#42b7ff";

    ctx.lineWidth = 2;


    ctx.beginPath();


    values.forEach(
        (value, index) => {

            const x =
                (
                    index /
                    (
                        values.length - 1
                    )
                ) *
                width;


            const y =
                height -
                25 -
                (
                    (
                        value -
                        minValue
                    ) /
                    (
                        maxValue -
                        minValue
                    )
                ) *
                (
                    height - 45
                );


            if (index === 0) {

                ctx.moveTo(
                    x,
                    y
                );

            } else {

                ctx.lineTo(
                    x,
                    y
                );
            }
        }
    );


    ctx.stroke();


    // ========================================================
    // GRAPH LABELS
    // ========================================================

    ctx.fillStyle =
        "#8ea2bb";

    ctx.font =
        "11px Segoe UI";


    ctx.fillText(
        "0 G",
        5,
        height - 8
    );


    ctx.fillText(
        maxValue.toFixed(1) + " G",
        5,
        14
    );
}


// ============================================================
// ESCAPE HTML FOR EVENT LOG
// ============================================================

function escapeHtml(value) {

    return String(value)

        .replaceAll(
            "&",
            "&amp;"
        )

        .replaceAll(
            "<",
            "&lt;"
        )

        .replaceAll(
            ">",
            "&gt;"
        )

        .replaceAll(
            '"',
            "&quot;"
        )

        .replaceAll(
            "'",
            "&#039;"
        );
}


// ============================================================
// EVENT LOG
// ============================================================

function renderLogs(logs) {

    const container =
        document.getElementById(
            "logs"
        );


    if (
        !logs ||
        logs.length === 0
    ) {

        container.innerHTML =
            '<div class="empty">' +
            'Waiting for serial events...' +
            '</div>';

        return;
    }


    container.innerHTML =
        logs
            .slice(0, 30)
            .map(
                log => `

                    <div class="log-item">

                        <span class="log-time">
                            ${escapeHtml(log.time)}
                        </span>

                        ${escapeHtml(log.message)}

                    </div>
                `
            )
            .join("");
}


// ============================================================
// UPDATE DASHBOARD
// ============================================================

async function updateDashboard() {

    try {

        const response =
            await fetch(
                "/api/status",
                {
                    cache: "no-store"
                }
            );


        const data =
            await response.json();


        // ====================================================
        // CONNECTION STATUS
        // ====================================================

        const online =
            data.connected;


        const dot =
            document.getElementById(
                "connectionDot"
            );


        const connectionText =
            document.getElementById(
                "connectionText"
            );


        dot.className =
            "dot " +
            (
                online
                    ? "online"
                    : "offline"
            );


        connectionText.textContent =
            online
                ? "ESP32 Connected"
                : "ESP32 Disconnected";


        // ====================================================
        // SERIAL PORT
        // ====================================================

        setText(
            "serialPort",
            data.port || "—"
        );


        // ====================================================
        // ACCIDENT
        // ====================================================

        setText(
            "accidentStatus",
            (
                data.accident ||
                "Normal"
            ).toUpperCase()
        );


        setStatusClass(
            data.accident ||
            "Normal"
        );


        // ====================================================
        // WARNING COUNTDOWN
        // ====================================================

        if (
            data.warning_seconds !== null &&
            data.warning_seconds !== undefined
        ) {

            setText(
                "warningText",
                `Cancellation window: ${data.warning_seconds}s`
            );

        } else {

            setText(
                "warningText",
                "No active warning"
            );
        }


        // ====================================================
        // MAIN METRICS
        // ====================================================

        setText(
            "impact",
            formatNumber(
                data.impact
            ) + " G"
        );


        setText(
            "accidentCount",
            data.accident_count ?? 0
        );


        setText(
            "satellites",
            data.satellites ?? "—"
        );


        // ====================================================
        // ACCELERATION
        // ====================================================

        setText(
            "ax",
            formatNumber(
                data.ax
            ) + " G"
        );


        setText(
            "ay",
            formatNumber(
                data.ay
            ) + " G"
        );


        setText(
            "az",
            formatNumber(
                data.az
            ) + " G"
        );


        // ====================================================
        // GPS
        // ====================================================

        setText(
            "latitude",
            data.latitude === null
                ? "—"
                : formatNumber(
                    data.latitude,
                    6
                )
        );


        setText(
            "longitude",
            data.longitude === null
                ? "—"
                : formatNumber(
                    data.longitude,
                    6
                )
        );


        setText(
            "altitude",
            data.altitude === null
                ? "—"
                : formatNumber(
                    data.altitude
                ) + " m"
        );


        setText(
            "speed",
            data.speed === null
                ? "—"
                : formatNumber(
                    data.speed
                ) + " km/h"
        );


        // ====================================================
        // SYSTEM HEALTH
        // ====================================================

        setText(
            "healthSerial",
            online
                ? "Connected"
                : "Offline"
        );


        setText(
            "healthMpu",
            data.mpu ||
            "Unknown"
        );


        setText(
            "healthGps",
            data.gps ||
            "Unknown"
        );


        setText(
            "healthWifi",
            data.wifi ||
            "Unknown"
        );


        setText(
            "healthTelegram",
            data.telegram ||
            "Unknown"
        );


        // ====================================================
        // LAST UPDATE
        // ====================================================

        if (data.last_update) {

            const age =
                Math.max(
                    0,
                    Math.floor(
                        Date.now() / 1000 -
                        data.last_update
                    )
                );


            setText(
                "lastUpdate",
                age + "s ago"
            );

        } else {

            setText(
                "lastUpdate",
                "—"
            );
        }


        // ====================================================
        // GPS BADGE
        // ====================================================

        const gpsBadge =
            document.getElementById(
                "gpsBadge"
            );


        gpsBadge.textContent =
            data.gps ||
            "WAITING";


        // ====================================================
        // GOOGLE MAPS BUTTON
        // ====================================================

        const mapButton =
            document.getElementById(
                "mapButton"
            );


        const validCoordinates =
            data.latitude !== null &&
            data.longitude !== null &&
            !Number.isNaN(
                Number(data.latitude)
            ) &&
            !Number.isNaN(
                Number(data.longitude)
            );


        mapButton.disabled =
            !validCoordinates;


        if (validCoordinates) {

            mapButton.onclick =
                () => {

                    const url =
                        `https://www.google.com/maps?q=` +
                        `${data.latitude},` +
                        `${data.longitude}`;


                    window.open(
                        url,
                        "_blank"
                    );
                };
        }


        // ====================================================
        // EVENT LOG
        // ====================================================

        renderLogs(
            data.logs
        );


        // ====================================================
        // GRAPH
        // ====================================================

        drawChart(
            data.graph
        );


    } catch (error) {

        const dot =
            document.getElementById(
                "connectionDot"
            );


        dot.className =
            "dot offline";


        setText(
            "connectionText",
            "Dashboard server unavailable"
        );
    }
}


// ============================================================
// WINDOW RESIZE
// ============================================================

window.addEventListener(
    "resize",
    () => {

        updateDashboard();

    }
);


// ============================================================
// START DASHBOARD UPDATES
// ============================================================

updateDashboard();


// Refresh every 1 second

setInterval(
    updateDashboard,
    1000
);