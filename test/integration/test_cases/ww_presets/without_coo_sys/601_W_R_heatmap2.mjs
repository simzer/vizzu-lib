import { data } from '../../../test_data/chart_types_eu.mjs';

const testSteps = [
    chart => chart.animate(
        {
            data: Object.assign(data, {
                filter: record =>
                    record.Country == 'Austria' ||
                    record.Country == 'Belgium' ||
                    record.Country == 'Bulgaria' ||
                    record.Country == 'Cyprus' ||
                    record.Country == 'Denmark' ||
                    record.Country == 'Estonia' ||
                    record.Country == 'Greece' ||
                    record.Country == 'Spain' ||
     //               record.Country == 'Finland' ||
     //               record.Country == 'France' ||
     //               record.Country == 'Croatia' ||
                   record.Country == 'Hungary'
            }),
            config:
            {
                channels:
                {
                    x: { set: 'Year' },
                    y: { set: 'Country_code' },
                    color: { set: 'Value 5 (+/-)', range:{min:'-60', max:'73'} }
                },
                title: 'Heatmap with Color Gradient'
            },
            style: {
                plot: {
                    paddingLeft: '0em',
                    marker: {
                        rectangleSpacing: 0,
                        colorGradient: '#3d51b8 0, #6389ec 0.15, #9fbffa 0.35, #d5d7d9 0.5, #f4b096 0.65, #e36c56 0.85, #ac1727 1',
                        },
                        xAxis: {
                            label: {
                               paddingTop: '0.8em'
                            }
                        },
                        yAxis: {
                            label: {
                               paddingRight: '0.8em'
                            }
                        }
                    },
                legend:{ maxWidth: '20%' },
            }
        }
    ),
    chart => chart.feature('tooltip',true)
];

export default testSteps;