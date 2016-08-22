
import injectTapEventPlugin from 'react-tap-event-plugin';

// Needed for onTouchTap
// Can go away when react 1.0 release
// Check this repo:
// https://github.com/zilverline/react-tap-event-plugin
injectTapEventPlugin();

let app = 'awesome';
var React = require('react');
var ReactDOM = require('react-dom');

import ThemeManager from 'material-ui/styles/themeManager';
import DarkRawTheme from 'material-ui/styles/raw-themes/dark-raw-theme';
import SliderMod from './Slider';
import Toggle from 'material-ui/Toggle';
import RaisedButton from 'material-ui/RaisedButton';

var ws;
var setters = {};


var sigFigs = function( val, min, max )
{
   var sensibility = (max-min)/5000;
   var logSensib = Math.round(Math.log10(sensibility));
   var logValue = Math.round(Math.log10(Math.abs(val)));
   if(logValue != logValue)
   {
    logValue = logSensib;
   }
   var power10 = Math.pow(10,Math.round(logSensib));
   
//   Math.round(val/power10)*power10)
  return val.toPrecision(Math.max(logValue-logSensib,1));
  }
  


var Generator = React.createClass({


  getInitialState: function()
  {
    let newMuiTheme = ThemeManager.getMuiTheme(DarkRawTheme);
    newMuiTheme.slider.handleSize = 2 * newMuiTheme.slider.handleSize;
    newMuiTheme.slider.handleSizeActive = 2 * newMuiTheme.slider.handleSizeActive;
    newMuiTheme.slider.trackSize = 2 * newMuiTheme.slider.handleSize;
    newMuiTheme.handleWhenPercentZero = newMuiTheme.handle;
    newMuiTheme.handleWhenPercentZeroAndDisabled = newMuiTheme.handleWhenDisabled;
    newMuiTheme.handleWhenPercentZeroAndFocused = newMuiTheme.handle;
    newMuiTheme.rippleWhenPercentZero = newMuiTheme.ripple;
    return {muiTheme: newMuiTheme};
  },
  

  childContextTypes : {
    muiTheme: React.PropTypes.object,
  },

  getChildContext() {
    return {
      muiTheme: this.state.muiTheme,
    };
  },
  
  render: function()
  {
    var objNodes = parseJsonToNodes(this.props.json);
    return (<div  style = {{color: '#dddddd'}}>
              <RaisedButton style={{float:'right'}} label="REFRESH" primary={true} onTouchTap={function(){ws.send("update");}} />
              <hr style = {{clear:'both', visibility:'hidden', margin:'0', padding:'0'}}/>
              {objNodes}</div>);
  }

})

var Container = React.createClass({

  render: function()
  {    
    var objNodes = parseJsonToNodes(this.props.json.content);
    return(<div className = "container">
        <div className="container_name" style={{color:'#303030',background:DarkRawTheme.palette.borderColor}}> {this.props.json.name} </div>
        <div className="container_content">{objNodes}</div>
      </div>)
  }
});

var makeUpdateJson = function(ident,val)
{
  return {
    "type":"update", 
    "content":[ 
      { 
        id: ident,
        value: val
       } 
      ]};
}

var sliderContainerStyle = 
{
    overflow:"hidden",
    verticalAlign:"middle"
}

var sliderStyle = 
{
  overflow:"hidden", 
  margin: '0'
}

var sliderStyleStyle = 
{
    marginBottom:'0.1em', 
    marginTop:'0.5em'
}

var sliderValueDisplayStyle =
{
  width:"auto", 
  marginRight:'0.5em',
  width:'3em',
  textAlign: 'right',
  color: DarkRawTheme.palette.borderColor,
  float: "left",
  marginTop: "0.6em"
}

var IntSlider = React.createClass({

  getInitialState: function()
  {
    var that = this;
    setters[this.props.json.id] = (function(val){that.updateState(val);});
    return {"value": this.convertValue(this.props.json.value)};
  },
  
  updateState: function(newValue)
  {
    this.setState({value: newValue});
  },
  
  handleChange: function(e,newValue)
  {
    this.updateState(newValue);
    this.communicateChange(newValue);
  },
  
  communicateChange: function (newValue)
  {
    var data = makeUpdateJson(this.props.json.id, newValue);
    ws.send(JSON.stringify(data));
  },
  
  convertValue: function(value)
  {
      return parseInt(value);
   },
  
  render: function()
  {
    var output = {};
   
      if(this.props.json.hasOwnProperty("min") && this.props.json.hasOwnProperty("max"))
      { 
          var step = 1;        
        
        var displayedValue = this.state.value;
                
        output = (<div style={sliderContainerStyle}><div style={sliderValueDisplayStyle}>{displayedValue}</div><SliderMod name={this.props.json.id} defaultValue={this.state.value} step={step} min={this.props.json.min} max={this.props.json.max} label={this.props.json.name} onChange={this.handleChange} value={this.state.value} style={sliderStyle} sliderStyle={sliderStyleStyle}/></div>);    
          
      }
      else
      {
        output = (this.state.value);
      }
      
      
    return (<div className="parameter">
      <div className="parameter_name"> {this.props.json.name} </div>
      <div className="parameter_content ">{output}</div>
      </div>)    
  }
});

var FloatSlider = React.createClass({

  getInitialState: function()
  {
    var that = this;
    setters[this.props.json.id] = (function(val){that.updateState(val);});
    return {"value": this.convertValue(this.props.json.value)};
  },
  
  updateState: function(newValue)
  {
    this.setState({value: newValue});
  },
  
  handleChange: function(e,newValue)
  {
    this.updateState(newValue);
    this.communicateChange(newValue);
  },
  
  communicateChange: function (newValue)
  {
    var data = makeUpdateJson(this.props.json.id, newValue);
    ws.send(JSON.stringify(data));
  },
  
  convertValue: function(value)
  {
     return parseFloat(value);
  },
  
  render: function()
  {
    var output = {};
    
      if(this.props.json.hasOwnProperty("min") && this.props.json.hasOwnProperty("max"))
      { 
        var step = (this.props.json.max-this.props.json.min)/1000;
                
        var displayedValue = sigFigs(this.state.value,this.props.json.min,this.props.json.max);

        output = (<div style={sliderContainerStyle}><div style={sliderValueDisplayStyle}>{displayedValue}</div><SliderMod name={this.props.json.id} defaultValue={this.state.value} step={step} min={this.props.json.min} max={this.props.json.max} label={this.props.json.name} onChange={this.handleChange} value={this.state.value} style={sliderStyle} sliderStyle={sliderStyleStyle}/></div>);    
          
      }
      else
      {
        output = (this.state.value);
      }
      
        
    return (<div className="parameter">
      <div className="parameter_name"> {this.props.json.name} </div>
      <div className="parameter_content">{output}</div>
      </div>)    
  }
});




var BoolToggle = React.createClass({

  getInitialState: function()
  {
    var that = this;
    setters[this.props.json.id] = (function(val){that.updateState(val);});
    return {"value": this.convertValue(this.props.json.value)};
  },
  
  updateState: function(newValue)
  {
    this.setState({value: newValue});
  },
  
  handleChange: function(e,newValue)
  {
    this.updateState(newValue);
    this.communicateChange(newValue);
  },
  
  communicateChange: function (newValue)
  {
    var data = makeUpdateJson(this.props.json.id, newValue);
    ws.send(JSON.stringify(data));
  },
  
  convertValue: function(value)
  {
        return Boolean(value);
  },
  
  render: function()
  {
    var  output = (<Toggle  name= {this.props.json.id}  defaultToggled={this.state.value}  label={this.props.json.name} labelStyle={{display:'none'}} onToggle={this.handleChange} iconStyle={{float:'right', marginTop:'0.2em'}}/>);
  
        
    return (<div className="parameter toggle">
      <div className="parameter_name"> {this.props.json.name} </div>
      <div className="parameter_content">{output}</div>
      </div>);
  }
});

var Action = React.createClass({
  
  
  applyAction: function(e)
  {
    var data = makeUpdateJson(this.props.json.id, null);
    ws.send(JSON.stringify(data));
  },
 
  render: function()
  {
    
    return (<div className="parameter button">
      <div className="parameter_name"> {this.props.json.name} </div>
      <div className="parameter_content"><RaisedButton style={{float:"right"}} label="apply" secondary={true} onTouchTap={this.applyAction} /></div>
      </div>)    
  }
});


var parseJsonToNodes = function(json)
{
  if(json == null)
  {return null;}
  
  return json.map(function(obj)
        {
            
          var output = {};
            if(obj == null)
            {
                //don't do anything
                
            }else if (obj.type == "parameter") 
            {
               switch(obj.valueType)
              {
                case 'i':
                  output = (<IntSlider key={obj.id} json={obj}/>);
                  break;
                case 'f':
                  output = (<FloatSlider key={obj.id} json={obj}/>);
                case 'd':
                  output = (<FloatSlider key={obj.id} json={obj}/>);
                  break;
                case 'b':
                  output = (<BoolToggle key={obj.id} json={obj}/>);
                  break;
                case 'a':
                  output = (<Action key={obj.id} json={obj}/>);
                  break;
                default:
                  return String(obj.name);
                  break;
              }
              
            } else if (obj.type == "group")
            {
              output = (<Container key={obj.id} json={obj}/>);
            }
            return output;
        });
}


var jsonTest = JSON.parse('[  {  "name" : "Param1", "type" : "parameter", "value" : 0, "valueType" : "i"  },  { "max" : 2, "min" : 0.0099999997764825821, "name" : "Param2", "type" : "parameter", "value" : 1, "valueType" : "f" }, { "max" : 2, "min" : 0.0099999997764825821, "name" : "Param3", "type" : "parameter", "value" : 1, "valueType" : "f" }, { "content" : [ { "name" : "ParamInGroup", "type" : "parameter", "value" : "hello", "valueType" : "s" }, {  "name" : "Toggle", "type" : "parameter", "value" : 1, "valueType" : "b"  }], "name" : "MyGroup", "type" : "group" }]');

var getWSUrl = function(){
  var http_url = window.location.href;
  var parts = http_url.split("/");
  if(parts[0]!="http:"){ alert("we expect the url to be of type 'http:' instead of '"+parts[0]+"'");}
  parts[0]="ws:"
  var ws_url = parts.join("/");
  return ws_url;
}

//ask through websocket for the json that will define the interface

var a = function(){
  var ws_url = getWSUrl();
  
  if("WebSocket" in window)
  {
    ws = new WebSocket(ws_url);
    
    ws.onopen = function()
    {
      ws.send("send_interface");
    }
    
    ws.onmessage = function(evt)
    {
      var message = JSON.parse(evt.data);

      if(message.type == "interface")
      {
        console.log(message.content);
        ReactDOM.render(<Generator json={message.content}/>,document.getElementById('example'));
      }
      else if (message.type == "update")
      {
        for(var i =0; i<message.content.length; i++)
        {
          var paramUpdate = message.content[i];
          var attrId = paramUpdate["id"];
          
          var fun = setters[attrId];
          if(typeof fun === "function")
          {
            var newValue = paramUpdate["value"];
            setters[attrId](newValue);
          }
          console.log(paramUpdate);
          //change state of the specified attribute
        }
      }
      else
      {
      }
    }
    
    ws.onclose = function()
    {
      alert("webscocket connection is closed");
    }
     
  }
  else
  {
    alert("Websockets are not available in this browser.");
  }
}()

//ReactDOM.render(<Generator json={jsonTest}/>,document.getElementById('example'));

