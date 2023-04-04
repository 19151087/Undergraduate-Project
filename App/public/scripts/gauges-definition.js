function createTemperatureGauge() {
    var chart = new Highcharts.Chart({

        chart: {
            renderTo: 'gauge-temperature',
            type: 'gauge',
            plotBackgroundColor: null,
            plotBackgroundImage: null,
            plotBorderWidth: 0,
            plotShadow: false,
            height: '76%'
        },

        title: {
            text: null
            // text: 'Temperature'
        },

        pane: {
            startAngle: -120,
            endAngle: 119.9,
            background: null,
            center: ['50%', '66%'],
            size: '100%'
        },

        // the value axis
        yAxis: {
            min: 0,
            max: 40,
            tickPixelInterval: 72,
            tickPosition: 'inside',
            tickColor: Highcharts.defaultOptions.chart.backgroundColor || '#FFFFFF',
            tickLength: 30,
            tickWidth: 2,
            minorTickInterval: null,
            labels: {
                distance: 20,
                style: {
                    fontSize: '14px'
                }
            },
            plotBands: [{
                from: 0,
                to: 10,
                color: '#CFE8FF', // light blue
                thickness: 30
            }, {
                from: 10,
                to: 20,
                color: '#3C91E6', // blue
                thickness: 30
            }, {
                from: 20,
                to: 30,
                color: '#6ab04c', // green
                thickness: 30
            }, {
                from: 30,
                to: 40,
                color: '#e84118', // red
                thickness: 30
            }]
        },

        series: [{
            name: 'Temperature',
            data: [20],
            tooltip: {
                valueSuffix: ' &deg;C'
            },
            dataLabels: {
                format: '{y} &deg;C',
                borderWidth: 0,
                color: (
                    Highcharts.defaultOptions.title &&
                    Highcharts.defaultOptions.title.style &&
                    Highcharts.defaultOptions.title.style.color
                ) || '#333333',
                style: {
                    fontSize: '24px'
                }
            },
            dial: {
                radius: '80%',
                backgroundColor: 'gray',
                baseWidth: 12,
                baseLength: '0%',
                rearLength: '0%'
            },
            pivot: {
                backgroundColor: 'gray',
                radius: 6
            }

        }],


        exporting: {
            enabled: false
        },
        tooltip: {
            enabled: false
        },
        credits: {
            enabled: false
        }

    });


    return chart;
}

function createHumidityGauge() {
    var chart = new Highcharts.Chart({

        chart: {
            renderTo: 'gauge-humidity',
            type: 'gauge',
            plotBackgroundColor: null,
            plotBackgroundImage: null,
            plotBorderWidth: 0,
            plotShadow: false,
            height: '76%'
        },

        title: {
            text: null
            // text: 'Temperature'
        },

        pane: {
            startAngle: -120,
            endAngle: 119.9,
            background: null,
            center: ['50%', '66%'],
            size: '100%'
        },

        // the value axis
        yAxis: {
            min: 0,
            max: 100,
            tickPixelInterval: 72,
            tickPosition: 'inside',
            tickColor: Highcharts.defaultOptions.chart.backgroundColor || '#F9F9F9',
            tickLength: 30,
            tickWidth: 2,
            minorTickInterval: null,
            labels: {
                distance: 20,
                style: {
                    fontSize: '14px'
                }
            },
            plotBands: [{
                from: 0,
                to: 20,
                color: '#f0932b', // orange
                thickness: 30
            }, {
                from: 20,
                to: 40,
                color: '#ffbe76', // light orange
                thickness: 30
            }, {
                from: 40,
                to: 80,
                color: '#6ab04c', // green
                thickness: 30
            }, {
                from: 80,
                to: 100,
                color: '#7ed6df', // light blue
                thickness: 30
            }]
        },

        series: [{
            name: 'Temperature',
            data: [0],
            tooltip: {
                valueSuffix: ' &percnt; RH'
            },
            dataLabels: {
                format: '{y} &percnt;RH',
                borderWidth: 0,
                color: (
                    Highcharts.defaultOptions.title &&
                    Highcharts.defaultOptions.title.style &&
                    Highcharts.defaultOptions.title.style.color
                ) || '#333333',
                style: {
                    fontSize: '24px'
                }
            },
            dial: {
                radius: '80%',
                backgroundColor: 'gray',
                baseWidth: 12,
                baseLength: '0%',
                rearLength: '0%'
            },
            pivot: {
                backgroundColor: 'gray',
                radius: 6
            }

        }],


        exporting: {
            enabled: false
        },
        tooltip: {
            enabled: false
        },
        credits: {
            enabled: false
        }

    });


    return chart;
}

function renderIcons() {

    // PM 2.5 icon
    if (!this.series[0].icon) {
        this.series[0].icon = this.renderer.path(['M', -8, 0, 'L', 8, 0, 'M', 0, -8, 'L', 8, 0, 0, 8])
            .attr({
                stroke: '#ffffff',
                'stroke-linecap': 'round',
                'stroke-linejoin': 'round',
                'stroke-width': 3,
                zIndex: 10
            })
            .add(this.series[2].group);
    }
    this.series[0].icon.translate(
        this.chartWidth / 2 - 10,
        this.plotHeight / 2 - this.series[0].points[0].shapeArgs.innerR -
        (this.series[0].points[0].shapeArgs.r - this.series[0].points[0].shapeArgs.innerR) / 2
    );

    // PM 10 icon
    if (!this.series[1].icon) {
        this.series[1].icon = this.renderer.path(['M', -8, -8, 'L', 0, 0, -8, 8, 'M', 0, -8, 'L', 8, 0, 0, 8])
            .attr({
                stroke: '#ffffff',
                'stroke-linecap': 'round',
                'stroke-linejoin': 'round',
                'stroke-width': 3,
                zIndex: 10
            })
            .add(this.series[2].group);
    }
    this.series[1].icon.translate(
        this.chartWidth / 2 - 10,
        this.plotHeight / 2 - this.series[1].points[0].shapeArgs.innerR -
        (this.series[1].points[0].shapeArgs.r - this.series[1].points[0].shapeArgs.innerR) / 2
    );

    // PM 1 icon
    if (!this.series[2].icon) {
        this.series[2].icon = this.renderer.path(['M', 0, 8, 'L', 0, -8, 'M', -8, 0, 'L', 0, -8, 8, 0])
            .attr({
                stroke: '#ffffff',
                'stroke-linecap': 'round',
                'stroke-linejoin': 'round',
                'stroke-width': 3,
                zIndex: 10
            })
            .add(this.series[2].group);
    }
    this.series[2].icon.translate(
        this.chartWidth / 2 - 10,
        this.plotHeight / 2 - this.series[2].points[0].shapeArgs.innerR -
        (this.series[2].points[0].shapeArgs.r - this.series[2].points[0].shapeArgs.innerR) / 2
    );
}

function createPMGauge() {
    var chart = new Highcharts.Chart({

        chart: {
            renderTo: 'gauge-pm',
            type: 'solidgauge',
            height: '76%',
            events: {
                render: renderIcons
            }
        },

        title: {
            text: null
        },

        tooltip: {
            borderWidth: 0,
            backgroundColor: 'none',
            shadow: false,
            style: {
                fontSize: '16px'
            },
            valueSuffix: ' ug/m3',
            pointFormat: '{series.name}<br><span style="font-size:1.5em; color: {point.color}; font-weight: bold; font-family: Poppins">{point.y}</span>',
            positioner: function (labelWidth) {
                return {
                    x: (this.chart.chartWidth - labelWidth) / 2,
                    y: (this.chart.plotHeight / 2) - 20
                };
            }
        },

        pane: {
            startAngle: 0,
            endAngle: 360,
            background: [{ // Track for PM 2.5
                outerRadius: '112%',
                innerRadius: '88%',
                backgroundColor: '#FFE0D3',
                borderWidth: 0
            }, { // Track for PM 10
                outerRadius: '87%',
                innerRadius: '63%',
                backgroundColor: '#FFF2C6',
                borderWidth: 0
            }, { // Track for PM 1
                outerRadius: '62%',
                innerRadius: '38%',
                backgroundColor: '#EAE2F8',
                borderWidth: 0
            }]
        },

        yAxis: {
            min: 0,
            max: 50,
            lineWidth: 0,
            tickPositions: []
        },

        plotOptions: {
            solidgauge: {
                dataLabels: {
                    enabled: false
                },
                linecap: 'round',
                stickyTracking: false,
                rounded: true
            }
        },

        series: [{
            name: 'PM 2.5',
            data: [{
                color: '#FD7238',
                radius: '112%',
                innerRadius: '88%',
                y: 0
            }]
        }, {
            name: 'PM 10',
            data: [{
                color: '#FFCE26',
                radius: '87%',
                innerRadius: '63%',
                y: 0
            }]
        }, {
            name: 'PM 1',
            data: [{
                color: '#A55EEA',
                radius: '62%',
                innerRadius: '38%',
                y: 0
            }]
        }],

        exporting: {
            enabled: false
        },
        credits: {
            enabled: false
        }
    });

    return chart;

}
