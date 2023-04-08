// Create the charts when the web page loads
window.addEventListener('load', onload);

function onload(event) {
  chartT = createTemperatureChart();
  chartH = createHumidityChart();
  chartPM = createPMChart();
}

// Create Temperature Chart
function createTemperatureChart() {
  var chart = new Highcharts.Chart({
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
        text: 'Temperature Celsius Degrees'
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
  var chart = new Highcharts.Chart({
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
  var chart = new Highcharts.Chart({
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

/*
events: {
        load: function () {
          // set up the updating of the chart each second
          var series = this.series[0];
          var series2 = this.series[1];
          var series3 = this.series[2];
          setInterval(
            function () {
              var x = (new Date()).getTime(), // current time
                y = Math.floor((Math.random() * 40) + 1),
                z = Math.floor((Math.random() * 30) + 1),
                w = Math.floor((Math.random() * 50) + 1);
              series.addPoint([x, y], false, true);
              series2.addPoint([x, z], true, true);
              series3.addPoint([x, w], true, true);
            }, 1000);
        }
      }
*/
