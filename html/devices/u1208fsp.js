function u1208fsp(modName, container) {

    this.m_container = $(container);
    this.m_mod = modName;
    this.m_lastTimestamp = 0;
    this.m_history = [];
    this.m_updatePeriod = 1000; //ms
    
    this.Request = function (req, callback) {
        if (!req.subsystem) {
            req.subsystem = this.m_mod;
        }
        $.post('/flugegeheimen', JSON.stringify(req), callback, 'json');
    };
    
    this.BuildControls = function () {
        var html = '';
        html += '<h1>USB-1208FS-Plus stream A/D converter</h1>' +
        '<label>Frequency, Hz&nbsp;</label><input type="number" id="freq" min="1" max="1000" value="100">' +
        '<label>Points to display&nbsp;</label><input type="number" id="length" min="10" max="100000" value="10000" step="500">' +
        '<button class="btn btn-danger" id="startExperiment">Start Experiment</button>' +
        '<button class="btn btn-primary" id="stopExperiment">Stop Experiment</button>' +
        '<div style="height: 400px;" id="chart"></div>' +
        '<div id="chart_ctl"></div>' +
        '<h2 id="info"></h2>' +
        '';

        this.m_container.html(html);
        this.ConnectControls();
    };

    this.ConnectControls = function () {
        this.m_chart = new FlugChart('#chart', '#chart_ctl');
        $('#startExperiment').on('click', this, this.startExperiment);
        $('#stopExperiment').on('click', this, this.stopExperiment);
    };

    this.startExperiment = function (ev) {
        var self = ev.data;
        self.Request({
            reqtype: 'startAIn',
            freq: parseFloat($('#freq').val()),
            ch: [0, 1, 2, 3],
            ranges: [0, 0, 0, 0],
        }, function (data) {
            if (data.status != 'success') {
                alert(data.description)
            }
            self.m_timer = setInterval(function () {
                self.update({data: self});
            }, 1000);
        })
    };


    this.update = function (ev) {
        var self = ev.data;
        self.Request({
            reqtype: 'getData',
            length: parseInt($('#length').val())
        }, function (data) {
            if (data.status != 'success') {
                alert(data.description)
            }else{
                if('period' in data && 'data' in data){
                    var timeline = [0];
                    var length = data.period.length;
                    for(var i = length - 1; i >= 0; i--){
                        timeline.push(-data.period[i] + timeline[timeline.length - 1]);
                    }
                    var series = [];
                    var legend = [];
                    for(var i = 0; i < data.data.length; i++){
                        series.push(data.data[i]);
                        legend.push("ch" + i);
                    }
                    self.m_chart.LoadDataSeriesPair(timeline.reverse(), series);
                    self.m_chart.SetLabels(legend);
                    self.m_chart.Draw();
                }
            }
        })
    };


    this.stopExperiment = function (ev) {
        var self = ev.data;
        clearInterval(self.m_timer);
        self.Request({
            reqtype: 'stopAIn'
        }, function (data) {
            if (data.status != 'success') {
                alert(data.description)
            }
        })
    };





}
