/**
 * API providing a rosette winds
 * @param {String} canvasId ID of a <canvas> element
 * @param {Number} [opt_radius] Radius, default is 50
 * @param {Boolean} [opt_mps] Show units in mps, default is false
 */
RosaceWind = function(canvasId, opt_radius, opt_mps)
{
    this.canvasId = canvasId;
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext("2d");

    this.radius = opt_radius || 50;
    this.arrowPosition = [this.radius * 1.5, this.radius * 0.5];
    this.centerRadius = 5;
    this.arrowColor = "red";
    this.arrowHeadLength = 10;
    this.clickPerformed = false;
    this.mps = opt_mps || false;

    this.init = function()
    {
        this.canvas.addEventListener(RosaceWind.MOUSEDOWN, this.onMouseDownListener);
    };

    //**********************************************************
    //  Methods
    //**********************************************************

    this.onMouseDownListener = function(event)
    {
        var mouseX = this.getMouseX(event);
        var mouseY = this.getMouseY(event);
        var hypotenuse = Math.sqrt(Math.pow(mouseX - this.radius, 2) + Math.pow(mouseY - this.radius, 2));
        if (hypotenuse <= this.radius)
        {
            this.arrowPosition = [mouseX, mouseY];
            this.draw();

            if (!this.clickPerformed)
            {
                window.addEventListener(RosaceWind.MOUSEMOVE, this.onMouseMoveListener);
                window.addEventListener(RosaceWind.MOUSEUP, this.onMouseUpListener);
                this.clickPerformed = true;
            }
        }
    }.bind(this);

    this.onMouseMoveListener = function(event)
    {
        if (this.clickPerformed)
        {
            var mouseX = this.getMouseX(event);
            var mouseY = this.getMouseY(event);

            var hypotenuse = Math.sqrt(Math.pow(mouseX - this.radius, 2) + Math.pow(mouseY - this.radius, 2));
            if (hypotenuse <= this.radius)
            {
                this.arrowPosition = [mouseX, mouseY];
                this.draw();
            }
        }
    }.bind(this);

    this.onMouseUpListener = function(event)
    {
        if (this.clickPerformed)
        {
            this.clickPerformed = false;
            this.onMouseUpCallback();
            window.removeEventListener(RosaceWind.MOUSEMOVE, this.onMouseMoveListener);
            window.removeEventListener(RosaceWind.MOUSEUP, this.onMouseUpListener);
        }
    }.bind(this);

    this.onMouseUpCallback = function() {
    };

    this.getMouseX = function(event)
    {
        var rect = this.canvas.getBoundingClientRect();
        var left = rect.left;
        var borderWidth = (rect.width - this.canvas.width) / 2;
        return ((RosaceWind.haveMouseEvents ? event.clientX : event.changedTouches[0].clientX) - left - borderWidth);
    };

    this.getMouseY = function(event)
    {
        var rect = this.canvas.getBoundingClientRect();
        var top = rect.top;
        var borderHeight = (rect.height - this.canvas.height) / 2;
        return ((RosaceWind.haveMouseEvents ? event.clientY : event.changedTouches[0].clientY) - top - borderHeight);
    };

    this.getWindU = function()
    {
        return (this.arrowPosition[0] - this.radius) * (20 / this.radius);
    };

    this.getWindV = function()
    {
        return (this.radius - this.arrowPosition[1]) * (20 / this.radius);
    };

    this.draw = function()
    {
        this.drawCircle();
        this.drawCenter();
        this.drawArrow();
        this.drawSpeed();
    };

    this.drawCircle = function()
    {
        var ctx = this.ctx;

        ctx.clearRect(0, 0, this.radius * 2, this.radius * 2);
        ctx.beginPath();
        ctx.arc(this.radius, this.radius, this.radius, 0, Math.PI * 2, false);
        ctx.fillStyle = "rgba(255, 255, 255, 0.4)";
        ctx.fill();
        ctx.lineWidth = 2;
        ctx.strokeStyle = "#000000";
        ctx.stroke();
    };

    this.drawCenter = function()
    {
        var ctx = this.ctx;

        ctx.beginPath();
        ctx.arc(this.radius, this.radius, this.centerRadius, 0, Math.PI * 2, false);
        ctx.fillStyle = "rgba(0, 0, 0, 0.3)";
        ctx.fill();
        ctx.lineWidth = 2;
        ctx.strokeStyle = "#000000";
        ctx.stroke();
    };

    this.drawArrow = function()
    {
        var fromx = this.radius;
        var fromy = this.radius;
        var tox = this.arrowPosition[0];
        var toy = this.arrowPosition[1];
        var angle = Math.atan2(toy - fromy, tox - fromx);
        var ctx = this.ctx;

        ctx.beginPath();
        ctx.moveTo(fromx, fromy);
        ctx.lineTo(tox, toy);
        ctx.lineTo(tox - this.arrowHeadLength * Math.cos(angle - Math.PI / 6), toy - this.arrowHeadLength * Math.sin(angle - Math.PI / 6));
        ctx.moveTo(tox, toy);
        ctx.lineTo(tox - this.arrowHeadLength * Math.cos(angle + Math.PI / 6), toy - this.arrowHeadLength * Math.sin(angle + Math.PI / 6));
        ctx.strokeStyle = this.arrowColor;
        ctx.lineWidth = 3;
        ctx.stroke();
    };

    this.drawSpeed = function()
    {
        var ctx = this.ctx;
        var val = (this.mps) ? Math.round(this.getSpeed()) + " m/s" : Math.round(this.getSpeed() * 3.6) + " km/h";

        ctx.font = "16px Helvetica";
        ctx.fillStyle = "black";
        ctx.fillText(val, this.radius - 20, this.radius - 10);

        ctx.fillText(Math.round(this.getAngle()) +"° ["+Math.round(this.getWindU())+","+Math.round(this.getWindV())+"]", this.radius - 40, this.radius + 17);
        
    };

    this.getSpeed = function()
    {
        return Math.sqrt(Math.pow(this.getWindU(), 2) + Math.pow(this.getWindV(), 2));
    };

    this.getAngle = function() {
        var windU = this.getWindU();
        var windV = this.getWindV();
    
        // Calculate the meteorological angle where the angle is measured from the north (0°) clockwise
        var meteorologyAngle = Math.atan2(-windV, -windU) * (180 / Math.PI);
        if (meteorologyAngle < 0) {
            meteorologyAngle += 360; // Normalize angle to be between 0° and 360°
        }
    
        // Convert to mathematical angle: 270° - meteorology angle
        var mathAngle = 270 - meteorologyAngle;
        
        // Ensure the math angle is positive
        if (mathAngle < 0) {
            mathAngle += 360;
        }
    
        return mathAngle;
    };
    this.setWindComponents = function(speed, meteorologyAngle) {
        var angleRadians = meteorologyAngle * (Math.PI / 180);
        this.windU = -speed * Math.sin(angleRadians);
        this.windV = -speed * Math.cos(angleRadians);
    };

    this.init();
};

if (document.ontouchmove !== undefined)
{
    RosaceWind.haveMouseEvents = false;
    RosaceWind.MOUSEMOVE = "touchmove";
    RosaceWind.MOUSEDOWN = "touchstart";
    RosaceWind.MOUSEUP = "touchend";
}
else
{
    RosaceWind.MOUSEMOVE = "mousemove";
    RosaceWind.MOUSEDOWN = "mousedown";
    RosaceWind.MOUSEUP = "mouseup";
    RosaceWind.haveMouseEvents = true;
}
