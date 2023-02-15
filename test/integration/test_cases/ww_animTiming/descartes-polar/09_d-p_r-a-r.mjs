import { data } from '../../../test_data/chart_types_eu.mjs';


const testSteps = [
    chart => chart.animate({
      data: Object.assign(data, {
            filter: record =>
                record.Country == 'Austria' ||
                record.Country == 'Belgium' ||
                record.Country == 'Bulgaria' ||
                record.Country == 'Cyprus' ||
                record.Country == 'Czechia' ||
                record.Country == 'Denmark' ||
                record.Country == 'Estonia' ||
                record.Country == 'Greece' ||
                record.Country == 'Germany' ||
                record.Country == 'Spain' ||
                record.Country == 'Finland' ||
                record.Country == 'France' ||
                record.Country == 'Croatia' ||
               record.Country == 'Hungary'
        }),,
        config: {
            channels: {
                x: { set: ['Year'] },
                y: { set: ['Country', 'Value 2 (+)'] },
                color: { set: ['Country'] },
            },
            title: 'Stacked Column Chart',
            geometry: 'rectangle',
            legend: null
        }
    }),


    chart => chart.animate({
        config: {
            channels: {
                x: { set: ['Year'] },
                y: { set: ['Value 2 (+)', 'Country'] },
                color: { set: ['Country'] },
            },
            title: 'Radial Area Chart',
            geometry: 'area',
            coordSystem: 'polar',
//            orientation: 'horizontal',
            split: false,
        }
    },
       {
        duration: 2.5,
        easing: 'cubic-bezier(.39,0,.35,.99)',
            coordSystem: {
                delay: 0,
                duration: 1,
            },
            geometry: { 
                delay: 0.5, 
                duration: 0.5
            },
            x: {
                delay: 0,
                duration: 0.5
            }, 
            y: {
                delay: 0.5,
                duration: 0.5
            }
        }
    ),

    
    chart => chart.animate({
    config: {
        channels: {
            x: { set: ['Year'] },
            y: { set: ['Country', 'Value 2 (+)'] },
            color: { set: ['Country'] },
        },
        title: 'Stacked Column Chart',
        geometry: 'rectangle',
        coordSystem: 'cartesian',
        legend: null
    } 
},
{
     geometry: { 
         delay: 0.7, 
         duration: 0.5
     },
     x: {
         delay: 1
     }
 }

)];

export default testSteps;