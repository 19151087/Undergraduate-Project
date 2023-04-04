// convert epochtime to JavaScripte Date object
function epochToJsDate(epochTime) {
  return new Date(epochTime * 1000);
}

// convert time to human-readable format YYYY/MM/DD HH:MM:SS
function epochToDateTime(epochTime) {
  var epochDate = new Date(epochToJsDate(epochTime));
  var dateTime = epochDate.getFullYear() + "/" +
    ("00" + (epochDate.getMonth() + 1)).slice(-2) + "/" +
    ("00" + epochDate.getDate()).slice(-2) + " " +
    ("00" + epochDate.getHours()).slice(-2) + ":" +
    ("00" + epochDate.getMinutes()).slice(-2) + ":" +
    ("00" + epochDate.getSeconds()).slice(-2);

  return dateTime;
}

// function to plot values on charts
function plotValues(chart, series_index, timestamp, value) {
  var x = epochToJsDate(timestamp).getTime();
  var y = Number(value);
  if (chart.series[series_index].data.length > 40) {
    chart.series[series_index].addPoint([x, y], true, true, true);
  } else {
    chart.series[series_index].addPoint([x, y], true, false, true);
  }
}

/*===================Document Object Model (DOM)=====================*/
const loginElement = document.querySelector('.login-form');
const contentElement = document.querySelector('#content');
const userDetailsElement = document.querySelector('#user-details');
const authBarElement = document.querySelector('.authentication-bar');
const deleteButtonElement = document.getElementById('delete-button');
const deleteModalElement = document.getElementById('delete-modal');
const deleteDataFormElement = document.querySelector('#delete-data-form');
const tableContainerElement = document.querySelector('#table-container');
const chartsRangeInputElement = document.getElementById('charts-range');
const loadDataButtonElement = document.getElementById('load-data');

// DOM elements for checkbox enabled/disabled
const cardsReadingsElement = document.querySelector(".cards-div");
const gaugesReadingsElement = document.querySelector(".gauges-div");
const chartsDivElement = document.querySelector('.chart-table-div');

// DOM elements for update data
const tempElement = document.getElementById("temp");
const humElement = document.getElementById("hum");
const pm2p5Element = document.getElementById("pm2p5");
const pm10Element = document.getElementById("pm10");
const pm1Element = document.getElementById("pm1");

const updateElement = document.getElementById("lastUpdate")

// MANAGE LOGIN/ LOGOUT UI
const setupUI = (user) => {
  if (user) {
    //toggle UI elements
    loginElement.style.display = 'none';
    contentElement.style.display = 'block';
    authBarElement.style.display = 'block';
    userDetailsElement.style.display = 'block';
    userDetailsElement.innerHTML = user.email;

    // Database paths
    var dbPath = 'dataSensor/Indoor';
    var chartPath = 'dataSensor/charts/range';

    // Database references
    var dbRef = firebase.database().ref(dbPath);
    var chartRef = firebase.database().ref(chartPath);

    // CARDS
    // Get the latest readings and display on cards
    dbRef.orderByKey().limitToLast(1).on('child_added', snapshot => {
      var jsonData = snapshot.toJSON(); // example: {temperature: 25.02, humidity: 50.20, pressure: 1008.48, timestamp:1641317355}
      var temperature = Math.round(jsonData.Temperature * 100) / 100;
      var humidity = Math.round(jsonData.Humidity * 100) / 100;
      var pm2p5 = jsonData.PM2_5;
      var pm10 = jsonData.PM10;
      var pm1 = jsonData.PM1_0;
      var timestamp = jsonData.Timestamp;
      // Update DOM elements
      tempElement.innerHTML = temperature;
      humElement.innerHTML = humidity;
      pm2p5Element.innerHTML = pm2p5;
      pm10Element.innerHTML = pm10;
      pm1Element.innerHTML = pm1;
      updateElement.innerHTML = epochToDateTime(timestamp);
    });

    // GAUGES
    // Get the latest readings and display on gauges
    dbRef.orderByKey().limitToLast(1).on('child_added', snapshot => {
      var jsonData = snapshot.toJSON();
      var temperature = Math.round(jsonData.Temperature * 100) / 100;
      var humidity = Math.round(jsonData.Humidity * 100) / 100;
      var pm2p5 = jsonData.PM2_5;
      var pm10 = jsonData.PM10;
      var pm1 = jsonData.PM1_0;

      var gaugeT = createTemperatureGauge();
      var gaugeH = createHumidityGauge();
      var guagePM = createPMGauge();

      gaugeT.series[0].points[0].update(temperature);
      gaugeH.series[0].points[0].update(humidity);
      guagePM.series[0].points[0].update(pm2p5);
      guagePM.series[1].points[0].update(pm10);
      guagePM.series[2].points[0].update(pm1);
    });

    // CHARTS
    // Number of readings to plot on charts
    var chartRange = 5;
    // Get number of readings to plot saved on database (runs when the page first loads and whenever there's a change in the database)
    chartRef.on('value', snapshot => {
      chartRange = Number(snapshot.val());
      console.log(chartRange);
      /* Delete all data from charts to update with new values when a new range is selected */
      chartT.destroy();
      chartH.destroy();
      // chartPM.destroy();
      /*Render new charts to display new range of data*/
      chartT = createTemperatureChart();
      chartH = createHumidityChart();
      chartPM = createPMChart();
      // Update the charts with the new range
      // Get the latest readings and plot them on charts (the number of plotted readings corresponds to the chartRange value)
      dbRef.orderByKey().limitToLast(chartRange).on('child_added', snapshot => {
        var jsonData = snapshot.toJSON();
        /*Save values on variables*/
        var temperature = jsonData.Temperature;
        var humidity = jsonData.Humidity;
        var pm2p5 = jsonData.PM2_5;
        var pm10 = jsonData.PM10;
        var pm1 = jsonData.PM1_0;
        var timestamp = jsonData.Timestamp;
        /*Plot the values on the charts*/
        plotValues(chartT, 0, timestamp, temperature);
        plotValues(chartH, 0, timestamp, humidity);
        plotValues(chartPM, 0, timestamp, pm2p5);
        plotValues(chartPM, 1, timestamp, pm10);
        plotValues(chartPM, 2, timestamp, pm1);
      });
    });
    // Update database with new range (input field)
    chartsRangeInputElement.onchange = () => {
      chartRef.set(chartsRangeInputElement.value);
    };

    // TABLE
    var lastReadingTimestamp; //saves last timestamp displayed on the table
    // append all data to the table
    var firstRun = true;
    dbRef.orderByKey().limitToLast(40).on('child_added', function (snapshot) {
      if (snapshot.exists()) {
        var jsonData = snapshot.toJSON();
        console.log(jsonData);
        var temperature = Math.round(jsonData.Temperature * 100) / 100;
        var humidity = Math.round(jsonData.Humidity * 100) / 100;
        var pm2p5 = jsonData.PM2_5;
        var pm10 = jsonData.PM10;
        var pm1 = jsonData.PM1_0;
        var timestamp = jsonData.Timestamp;
        var content = '';
        content += '<tr>';
        content += '<td>' + epochToDateTime(timestamp) + '</td>';
        content += '<td>' + temperature + '</td>';
        content += '<td>' + humidity + '</td>';
        content += '<td>' + pm2p5 + '</td>';
        content += '<td>' + pm10 + '</td>';
        content += '<td>' + pm1 + '</td>';
        content += '</tr>';
        $('#tbody').prepend(content);
        // Save lastReadingTimestamp --> corresponds to the first timestamp on the returned snapshot data
        if (firstRun) {
          lastReadingTimestamp = timestamp;
          firstRun = false;
          console.log(lastReadingTimestamp);
        }
      }
    });

    // append readings to table (after pressing More results... button)
    async function appendToTable() {
      var dataList = []; // saves list of readings returned by the snapshot (oldest-->newest)
      var reversedList = []; // the same as previous, but reversed (newest--> oldest)
      console.log("APEND");
      await dbRef.orderByKey().limitToLast(200).endAt(String(lastReadingTimestamp)).once('value', function (snapshot) {
        // convert the snapshot to JSON
        if (snapshot.exists()) {
          snapshot.forEach(element => {
            var jsonData = element.toJSON();
            dataList.push(jsonData); // create a list with all data
          });
          lastReadingTimestamp = dataList[0].timestamp; //oldest timestamp corresponds to the first on the list (oldest --> newest)
          reversedList = dataList.reverse(); // reverse the order of the list (newest data --> oldest data)

          var firstTime = true;
          // loop through all elements of the list and append to table (newest elements first)
          reversedList.forEach(element => {
            if (firstTime) { // ignore first reading (it's already on the table from the previous query)
              firstTime = false;
            }
            else {
              var temperature = Math.round(element.Temperature * 100) / 100;
              var humidity = Math.round(element.Humidity * 100) / 100;
              var pm2p5 = element.PM2_5;
              var pm10 = element.PM10;
              var pm1 = element.PM1_0;
              var timestamp = element.Timestamp;
              var content = '';
              content += '<tr>';
              content += '<td>' + epochToDateTime(timestamp) + '</td>';
              content += '<td>' + temperature + '</td>';
              content += '<td>' + humidity + '</td>';
              content += '<td>' + pm2p5 + '</td>';
              content += '<td>' + pm10 + '</td>';
              content += '<td>' + pm1 + '</td>';
              content += '</tr>';
              $('#tbody').append(content);
            }
          });
        }
      });
      if (dataList.length < 40) {
        loadDataButtonElement.style.display = "none";
      }
      else {
        loadDataButtonElement.style.display = "block";
      }
    }

    // DELETE DATA
    // Add event listener to open modal when click on "Delete Data" button
    deleteButtonElement.addEventListener('click', e => {
      console.log("Remove data");
      e.preventDefault;
      deleteModalElement.style.display = "block";
    });

    // Add event listener when delete form is submited
    deleteDataFormElement.addEventListener('submit', (e) => {
      // delete data (readings)
      dbRef.remove();
    });

    loadDataButtonElement.addEventListener('click', (e) => {
      appendToTable();
    });

    // IF USER IS LOGGED OUT
  } else {
    // toggle UI elements
    loginElement.style.display = 'block';
    authBarElement.style.display = 'none';
    userDetailsElement.style.display = 'none';
    contentElement.style.display = 'none';
  }
}
