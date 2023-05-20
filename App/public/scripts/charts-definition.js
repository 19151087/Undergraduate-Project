// Create the charts when the web page loads
window.addEventListener('load', onload);

function onload(event) {
  chartT = createTemperatureChart();
  chartH = createHumidityChart();
  chartPM = createPMChart();
}

// Create Temperature Chart
function createTemperatureChart() {
  let chart = new Highcharts.Chart({
    chart: {
      renderTo: 'chart-temperature',
      type: 'spline'
    },
    time: {
      useUTC: false
    },
    series: [{ name: 'Temperature' }],
    title: {
      text: 'Graph of temperature changing over time'
    },
    plotOptions: {
      line: {
        animation: false,
        dataLabels: {
          enabled: false
        }
      }
    },
    xAxis: {
      type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
      title: {
        text: 'Temperature Celsius Degrees (°C)'
      }
    },
    tooltip: {
      headerFormat: '<b>{series.name}</b><br/>',
      pointFormat: '{point.x:%Y-%m-%d %H:%M:%S}<br/>{point.y:.2f}'
    },
    accessibility: {
      announceNewData: {
        enabled: true,
        minAnnounceInterval: 15000,
        announcementFormatter: function (allSeries, newSeries, newPoint) {
          if (newPoint) {
            return 'New point added. Value: ' + newPoint.y;
          }
          return false;
        }
      }
    },
    exporting: {
      enabled: false
    },
    credits: {
      enabled: false
    }
  });
  return chart;
}

// Create Humidity Chart
function createHumidityChart() {
  let chart = new Highcharts.Chart({
    chart: {
      renderTo: 'chart-humidity',
      type: 'spline'
    },
    time: {
      useUTC: false
    },
    series: [{ name: 'Humidity' }],
    title: {
      text: 'Graph of humidity changing over time'
    },
    plotOptions: {
      line: {
        animation: false,
        dataLabels: {
          enabled: false
        }
      },
      series: {
        color: '#50b8b4'
      }
    },
    xAxis: {
      type: 'datetime',
      dateTimeLabelFormats: { second: '%H:%M:%S' }
    },
    yAxis: {
      title: {
        text: 'Humidity (%)'
      }
    },
    tooltip: {
      headerFormat: '<b>{series.name}</b><br/>',
      pointFormat: '{point.x:%Y-%m-%d %H:%M:%S}<br/>{point.y:.2f}'
    },
    accessibility: {
      announceNewData: {
        enabled: true,
        minAnnounceInterval: 15000,
        announcementFormatter: function (allSeries, newSeries, newPoint) {
          if (newPoint) {
            return 'New point added. Value: ' + newPoint.y;
          }
          return false;
        }
      }
    },
    exporting: {
      enabled: false
    },
    credits: {
      enabled: false
    }
  });
  return chart;
}

// Create PM Chart
function createPMChart() {
  let chart = new Highcharts.Chart({
    chart: {
      renderTo: 'chart-pm',
      type: 'spline',
      // animation: Highcharts.svg, // don't animate in old IE
      // marginRight: 10
    },
    time: {
      useUTC: false
    },
    title: {
      text: 'Chart of particular matter index in the air over time'
    },
    accessibility: {
      announceNewData: {
        enabled: true,
        minAnnounceInterval: 15000,
        announcementFormatter: function (allSeries, newSeries, newPoint) {
          if (newPoint) {
            return 'New point added. Value: ' + newPoint.y;
          }
          return false;
        }
      }
    },
    xAxis: {
      type: 'datetime',
      tickPixelInterval: 150
    },
    yAxis: [{
      title: {
        text: 'Particulate Matter (ug/m3)'
      },
      plotLines: [{
        value: 0,
        width: 1,
        color: '#808080'
      }]
    }],
    tooltip: {
      headerFormat: '<b>{series.name}</b><br/>',
      pointFormat: '{point.x:%Y-%m-%d %H:%M:%S}<br/>{point.y}'
    },
    series: [
      { name: 'PM 2.5' },
      { name: 'PM 10' },
      { name: 'PM 1' }
    ],
    legend: {
      enabled: true
    },
    exporting: {
      enabled: false
    },
    credits: {
      enabled: false
    }
  });
  return chart;
}